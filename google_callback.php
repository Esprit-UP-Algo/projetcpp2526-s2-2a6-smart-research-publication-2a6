<?php
// Google OAuth callback endpoint.
// Exchanges authorization code for access token, then fetches user profile.

// Optional local config file for development.
// It can define GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET, GOOGLE_REDIRECT_URI constants.
$localConfig = __DIR__ . '/google_oauth_config.php';
if (is_file($localConfig)) {
    require_once $localConfig;
}

session_start();

function getEnvTrimmed(string $name): string
{
    $value = getenv($name);
    if ($value === false) return '';
    return trim((string)$value);
}

function getConfigValue(string $envName, string $constName): string
{
    $fromEnv = getEnvTrimmed($envName);
    if ($fromEnv !== '') {
        return $fromEnv;
    }

    if (defined($constName)) {
        return trim((string)constant($constName));
    }

    return '';
}

function currentBaseUrl(): string
{
    $isHttps = (
        (!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off')
        || (isset($_SERVER['SERVER_PORT']) && (int)$_SERVER['SERVER_PORT'] === 443)
    );

    $scheme = $isHttps ? 'https' : 'http';
    $host = $_SERVER['HTTP_HOST'] ?? '127.0.0.1:8000';

    return $scheme . '://' . $host;
}

function postForm(string $url, array $form): array
{
    $options = [
        'http' => [
            'method' => 'POST',
            'header' => "Content-Type: application/x-www-form-urlencoded\r\n",
            'content' => http_build_query($form),
            'timeout' => 20,
            'ignore_errors' => true
        ]
    ];

    $context = stream_context_create($options);
    $raw = @file_get_contents($url, false, $context);
    if ($raw === false) {
        return ['ok' => false, 'body' => '', 'status' => 0];
    }

    $status = 0;
    if (isset($http_response_header) && is_array($http_response_header) && isset($http_response_header[0])) {
        if (preg_match('/\s(\d{3})\s/', $http_response_header[0], $m)) {
            $status = (int)$m[1];
        }
    }

    return ['ok' => $status >= 200 && $status < 300, 'body' => $raw, 'status' => $status];
}

function getJson(string $url, array $headers = []): array
{
    $headerText = '';
    foreach ($headers as $h) {
        $headerText .= $h . "\r\n";
    }

    $options = [
        'http' => [
            'method' => 'GET',
            'header' => $headerText,
            'timeout' => 20,
            'ignore_errors' => true
        ]
    ];

    $context = stream_context_create($options);
    $raw = @file_get_contents($url, false, $context);
    if ($raw === false) {
        return ['ok' => false, 'body' => '', 'status' => 0];
    }

    $status = 0;
    if (isset($http_response_header) && is_array($http_response_header) && isset($http_response_header[0])) {
        if (preg_match('/\s(\d{3})\s/', $http_response_header[0], $m)) {
            $status = (int)$m[1];
        }
    }

    return ['ok' => $status >= 200 && $status < 300, 'body' => $raw, 'status' => $status];
}

function postJson(string $url, array $payload): array
{
    $options = [
        'http' => [
            'method' => 'POST',
            'header' => "Content-Type: application/json\r\n",
            'content' => json_encode($payload, JSON_UNESCAPED_UNICODE),
            'timeout' => 5,
            'ignore_errors' => true
        ]
    ];

    $context = stream_context_create($options);
    $raw = @file_get_contents($url, false, $context);
    if ($raw === false) {
        return ['ok' => false, 'body' => '', 'status' => 0];
    }

    $status = 0;
    if (isset($http_response_header) && is_array($http_response_header) && isset($http_response_header[0])) {
        if (preg_match('/\s(\d{3})\s/', $http_response_header[0], $m)) {
            $status = (int)$m[1];
        }
    }

    return ['ok' => $status >= 200 && $status < 300, 'body' => $raw, 'status' => $status];
}

function renderResult(string $title, string $message, bool $ok, array $profile = [], bool $qtNotified = false): void
{
    $badge = $ok ? '#0b7a4f' : '#9e2f45';
    $label = $ok ? 'SUCCES' : 'ERREUR';

    echo '<!DOCTYPE html><html lang="fr"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">';
    echo '<title>' . htmlspecialchars($title, ENT_QUOTES, 'UTF-8') . '</title>';
    echo '<style>body{margin:0;font-family:Segoe UI,Arial,sans-serif;background:#eef6f4;color:#1f3f3a;display:grid;min-height:100vh;place-items:center;padding:20px}.card{max-width:760px;width:100%;background:#fff;border:1px solid #d9e8e5;border-radius:14px;padding:22px;box-shadow:0 12px 26px rgba(0,0,0,.08)}.b{display:inline-block;padding:4px 10px;border-radius:999px;color:#fff;font-size:12px;font-weight:700;background:' . $badge . '}h1{margin:10px 0;color:#0a5f58}pre{background:#f2f7f6;border:1px solid #d9e8e5;padding:10px;border-radius:8px;overflow:auto}</style>';
    echo '</head><body><div class="card">';
    echo '<span class="b">' . $label . '</span>';
    echo '<h1>' . htmlspecialchars($title, ENT_QUOTES, 'UTF-8') . '</h1>';
    echo '<p>' . htmlspecialchars($message, ENT_QUOTES, 'UTF-8') . '</p>';

    if (!empty($profile)) {
        echo '<pre>' . htmlspecialchars(json_encode($profile, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE), ENT_QUOTES, 'UTF-8') . '</pre>';
    }

    if ($ok && $qtNotified) {
        echo '<p>La connexion a ete transmise a l application Qt. Cette fenetre peut etre fermee.</p>';
        echo '<script>setTimeout(function(){ window.close(); }, 1200);</script>';
    } elseif ($ok) {
        echo '<p>Tu peux revenir à l application Qt. La session web contient le profil Google.</p>';
    }

    echo '</div></body></html>';
}

$clientId = getConfigValue('GOOGLE_CLIENT_ID', 'GOOGLE_CLIENT_ID');
$clientSecret = getConfigValue('GOOGLE_CLIENT_SECRET', 'GOOGLE_CLIENT_SECRET');
$redirectUri = getConfigValue('GOOGLE_REDIRECT_URI', 'GOOGLE_REDIRECT_URI');
if ($redirectUri === '') {
    $redirectUri = currentBaseUrl() . '/google_callback.php';
}

if ($clientId === '' || $clientSecret === '') {
    renderResult('Configuration manquante', 'GOOGLE_CLIENT_ID ou GOOGLE_CLIENT_SECRET est vide.', false);
    exit;
}

$incomingState = isset($_GET['state']) ? (string)$_GET['state'] : '';
$sessionState = isset($_SESSION['oauth2_state']) ? (string)$_SESSION['oauth2_state'] : '';

if ($incomingState === '' || $sessionState === '' || !hash_equals($sessionState, $incomingState)) {
    renderResult('State invalide', 'Protection CSRF: state OAuth invalide.', false);
    exit;
}

if (!isset($_GET['code']) || trim((string)$_GET['code']) === '') {
    $error = isset($_GET['error']) ? (string)$_GET['error'] : 'code manquant';
    renderResult('Autorisation refusée', 'Google n a pas fourni de code: ' . $error, false);
    exit;
}

$code = trim((string)$_GET['code']);

$tokenResponse = postForm('https://oauth2.googleapis.com/token', [
    'code' => $code,
    'client_id' => $clientId,
    'client_secret' => $clientSecret,
    'redirect_uri' => $redirectUri,
    'grant_type' => 'authorization_code'
]);

if (!$tokenResponse['ok']) {
    renderResult('Echange token échoué', 'Impossible d obtenir le token Google (HTTP ' . $tokenResponse['status'] . ').', false);
    exit;
}

$tokenData = json_decode($tokenResponse['body'], true);
if (!is_array($tokenData) || empty($tokenData['access_token'])) {
    renderResult('Token invalide', 'Réponse token Google invalide.', false);
    exit;
}

$accessToken = (string)$tokenData['access_token'];

$userInfoResponse = getJson('https://www.googleapis.com/oauth2/v3/userinfo', [
    'Authorization: Bearer ' . $accessToken
]);

if (!$userInfoResponse['ok']) {
    renderResult('Profil Google indisponible', 'Impossible de récupérer le profil Google (HTTP ' . $userInfoResponse['status'] . ').', false);
    exit;
}

$profile = json_decode($userInfoResponse['body'], true);
if (!is_array($profile)) {
    renderResult('Profil invalide', 'La réponse userinfo est invalide.', false);
    exit;
}

$_SESSION['google_user'] = [
    'sub' => $profile['sub'] ?? '',
    'email' => $profile['email'] ?? '',
    'name' => $profile['name'] ?? '',
    'picture' => $profile['picture'] ?? ''
];

$qtNotified = false;
$qtCallback = isset($_SESSION['oauth2_qt_callback']) ? trim((string)$_SESSION['oauth2_qt_callback']) : '';
if ($qtCallback !== '') {
    $qtUrl = filter_var($qtCallback, FILTER_VALIDATE_URL) ? $qtCallback : '';
    if ($qtUrl !== '') {
        $parts = parse_url($qtUrl);
        $scheme = isset($parts['scheme']) ? strtolower((string)$parts['scheme']) : '';
        $host = isset($parts['host']) ? strtolower((string)$parts['host']) : '';

        if ($scheme === 'http' && ($host === '127.0.0.1' || $host === 'localhost')) {
            $notify = postJson($qtUrl, [
                'email' => $_SESSION['google_user']['email'],
                'name' => $_SESSION['google_user']['name'],
                'sub' => $_SESSION['google_user']['sub'],
                'picture' => $_SESSION['google_user']['picture']
            ]);
            $qtNotified = (bool)$notify['ok'];
        }
    }
}

renderResult('Connexion Google réussie', 'Le profil Google a été reçu avec succès.', true, $_SESSION['google_user'], $qtNotified);
