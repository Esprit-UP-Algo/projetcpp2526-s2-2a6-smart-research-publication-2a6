<?php
session_start();

require_once '../config/Database.php';

header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    echo json_encode(['success' => false, 'message' => 'Méthode non autorisée.']);
    exit;
}

$data = json_decode(file_get_contents('php://input'), true);

if (!isset($data['face_embedding']) || !is_array($data['face_embedding'])
    || count($data['face_embedding']) < 10) {
    echo json_encode(['success' => false, 'message' => 'Embedding facial manquant ou invalide.']);
    exit;
}

// ─── Distance functions ──────────────────────────────────────────

/**
 * Euclidean distance between two 128-dim descriptors.
 * Face-api.js faceRecognitionNet already L2-normalises its output,
 * so Euclidean distance and cosine distance are equivalent:
 *   euclidean = sqrt(2 - 2 * cos_sim)
 * Threshold ≈ 0.60 is the standard for this network.
 */
function euclideanDistance(array $a, array $b): float
{
    if (count($a) !== count($b) || count($a) === 0) return INF;
    $sum = 0.0;
    for ($i = 0, $n = count($a); $i < $n; $i++) {
        $d = (float)$a[$i] - (float)$b[$i];
        $sum += $d * $d;
    }
    return sqrt($sum);
}

/**
 * Normalise a descriptor to unit length.
 * Guards against the rare case where the stored or input embedding
 * was not already normalised (e.g. rounded/truncated during storage).
 */
function l2Normalize(array $v): array
{
    $norm = 0.0;
    foreach ($v as $x) $norm += (float)$x * (float)$x;
    $norm = sqrt($norm);
    if ($norm < 1e-9) return $v;
    return array_map(fn($x) => (float)$x / $norm, $v);
}

/**
 * Given a stored face_embedding JSON string, return an array of
 * normalised 128-dim descriptors.
 *
 * Supports two storage formats:
 *   • Single embedding  : [0.12, -0.34, …]          → [[0.12, -0.34, …]]
 *   • Multiple embeddings: [[0.12, …], [0.08, …], …] → [[0.12, …], [0.08, …]]
 */
function parseStoredEmbedding(string $json): array
{
    $decoded = json_decode($json, true);
    if (!is_array($decoded) || empty($decoded)) return [];

    // Detect format by checking the first element
    if (is_array($decoded[0])) {
        // Multiple embeddings stored as array of arrays
        $result = [];
        foreach ($decoded as $emb) {
            if (is_array($emb) && count($emb) >= 10) {
                $result[] = l2Normalize(array_values($emb));
            }
        }
        return $result;
    }

    // Single flat embedding
    return [l2Normalize(array_values($decoded))];
}

// ─── Main verification ───────────────────────────────────────────

try {
    $inputRaw       = array_values($data['face_embedding']);
    $inputEmbedding = l2Normalize(array_map('floatval', $inputRaw));

    // Threshold: 0.60 is the face-api.js recommended value.
    // Front-end now sends an averaged 3-sample descriptor, which is
    // more representative than a single frame, so 0.60 is kept.
    $threshold = 0.60;

    $database = new Database();
    $db       = $database->getConnection();

    $stmt = $db->prepare(
        "SELECT id, email, face_embedding FROM users WHERE face_embedding IS NOT NULL"
    );
    $stmt->execute();
    $users = $stmt->fetchAll(PDO::FETCH_ASSOC);

    if (!$users) {
        echo json_encode([
            'success' => false,
            'message' => 'Aucun visage enregistré dans la base. Inscrivez-vous d\'abord.',
        ]);
        exit;
    }

    $bestUser     = null;
    $bestDistance = INF;

    foreach ($users as $user) {
        $storedEmbeddings = parseStoredEmbedding($user['face_embedding']);
        if (empty($storedEmbeddings)) continue;

        // If multiple embeddings are stored for this user (e.g. from several
        // registration shots), take the minimum distance — best match wins.
        foreach ($storedEmbeddings as $storedEmb) {
            $dist = euclideanDistance($inputEmbedding, $storedEmb);
            if ($dist < $bestDistance) {
                $bestDistance = $dist;
                $bestUser     = $user;
            }
        }
    }

    if ($bestUser !== null && $bestDistance < $threshold) {
        $_SESSION['user_id']    = $bestUser['id'];
        $_SESSION['user_email'] = $bestUser['email'];

        echo json_encode([
            'success'    => true,
            'message'    => 'Connexion Face ID réussie.',
            'redirect'   => 'dashboard.php',
            'confidence' => round((1 - $bestDistance / $threshold) * 100), // 0–100 %
        ]);
        exit;
    }

    // Not matched — give a useful hint based on how close we got
    $hint = '';
    if ($bestDistance !== INF) {
        if ($bestDistance < $threshold * 1.15) {
            $hint = ' La correspondance était proche — essayez avec un meilleur éclairage.';
        } elseif ($bestDistance < $threshold * 1.40) {
            $hint = ' Assurez-vous d\'être la même personne enregistrée.';
        }
    }

    echo json_encode([
        'success' => false,
        'message' => 'Visage non reconnu. Réessayez.' . $hint,
    ]);

} catch (Throwable $e) {
    echo json_encode([
        'success' => false,
        'message' => 'Erreur serveur pendant la vérification faciale.',
    ]);
}
