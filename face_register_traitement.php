<?php
session_start();

require_once '../config/Database.php';
require_once '../models/user.php';

header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] === 'POST') {

    // Verifier que l'utilisateur est en cours d'inscription
    if (!isset($_SESSION['registering_email'])) {
        echo json_encode([
            'success' => false,
            'message' => 'Session d\'inscription introuvable.'
        ]);
        exit;
    }

    $email = $_SESSION['registering_email'];

    // Recuperer les donnees JSON
    $data = json_decode(file_get_contents('php://input'), true);

    if (!isset($data['face_embedding']) || empty($data['face_embedding'])) {
        echo json_encode([
            'success' => false,
            'message' => 'Embedding facial manquant.'
        ]);
        exit;
    }

    $faceEmbedding = json_encode($data['face_embedding']);

    // Connexion BDD
    $database = new Database();
    $db = $database->getConnection();
    $userModel = new User($db);

    // Enregistrer l'embedding facial
    $result = $userModel->saveFaceEmbedding($email, $faceEmbedding);

    if ($result) {
        unset($_SESSION['registering_email']);

        echo json_encode([
            'success' => true,
            'message' => 'Face ID enregistre avec succes !'
        ]);
    } else {
        echo json_encode([
            'success' => false,
            'message' => 'Erreur lors de l\'enregistrement du Face ID.'
        ]);
    }

} else {
    echo json_encode([
        'success' => false,
        'message' => 'Methode non autorisee.'
    ]);
}
