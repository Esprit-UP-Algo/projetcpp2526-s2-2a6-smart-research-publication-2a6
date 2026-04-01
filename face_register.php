<?php
session_start();

// Verifier que l'utilisateur est en cours d'inscription
if (!isset($_SESSION['registering_email'])) {
    header("Location: register.html");
    exit;
}

$email = $_SESSION['registering_email'];
?>
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Enregistrement Face ID - Together4Peace</title>
    <link rel="stylesheet" href="../styles.css">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css">
    <style>
        .auth-page {
            background: linear-gradient(135deg, var(--color-primary) 0%, var(--color-accent) 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .auth-container {
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
            padding: 40px;
            width: 100%;
            max-width: 600px;
        }
        .auth-header {
            text-align: center;
            margin-bottom: 30px;
        }
        .auth-header h2 {
            color: var(--color-primary);
            margin-bottom: 10px;
        }
        #video {
            width: 100%;
            max-width: 500px;
            border-radius: 10px;
            margin: 20px auto;
            display: block;
            background: #000;
        }
        #canvas {
            display: none;
        }
        .face-detection-area {
            position: relative;
            margin: 20px 0;
            text-align: center;
        }
        .instructions {
            background: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            text-align: left;
        }
        .instructions ul {
            margin: 10px 0;
            padding-left: 20px;
        }
        .btn-face {
            background-color: var(--color-primary);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 8px;
            font-size: 1.1em;
            cursor: pointer;
            margin: 10px 5px;
            transition: all 0.3s;
        }
        .btn-face:hover {
            background-color: #001a3d;
        }
        .btn-face:disabled {
            background-color: #ccc;
            cursor: not-allowed;
        }
        .btn-secondary {
            background-color: #6c757d;
        }
        .btn-secondary:hover {
            background-color: #5a6268;
        }
        .status-message {
            padding: 12px;
            border-radius: 8px;
            margin: 15px 0;
            text-align: center;
            font-weight: 500;
        }
        .status-success {
            background-color: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .status-error {
            background-color: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .status-info {
            background-color: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }
        .button-group {
            text-align: center;
            margin-top: 20px;
        }
    </style>
</head>
<body class="auth-page">
    <div class="auth-container">
        <div class="auth-header">
            <img src="../logo.png" alt="Logo Together4Peace" class="auth-logo" style="height: 60px; margin-bottom: 20px;">
            <h2>Enregistrement Face ID</h2>
            <p>Scannez votre visage pour activer la connexion par reconnaissance faciale</p>
        </div>

        <div class="instructions">
            <strong>Instructions :</strong>
            <ul>
                <li>Assurez-vous d'avoir un bon eclairage</li>
                <li>Regardez directement la camera</li>
                <li>Gardez votre visage au centre du cadre</li>
                <li>Restez immobile pendant la capture</li>
            </ul>
        </div>

        <div class="face-detection-area">
            <video id="video" autoplay playsinline></video>
            <canvas id="canvas"></canvas>
            <div id="statusMessage" class="status-message" style="display: none;"></div>
        </div>

        <div class="button-group">
            <button id="startCamera" class="btn-face">
                <i class="fas fa-video"></i> Activer la camera
            </button>
            <button id="captureFace" class="btn-face" disabled>
                <i class="fas fa-camera"></i> Capturer le visage
            </button>
            <button id="saveFace" class="btn-face btn-secondary" disabled>
                <i class="fas fa-save"></i> Enregistrer Face ID
            </button>
            <button id="skipFace" class="btn-face btn-secondary">
                <i class="fas fa-forward"></i> Passer cette etape
            </button>
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/face-api.js@0.22.2/dist/face-api.min.js"></script>
    <script>
        let stream = null;
        let faceEmbedding = null;
        let modelsLoaded = false;

        const video = document.getElementById('video');
        const canvas = document.getElementById('canvas');
        const startCameraBtn = document.getElementById('startCamera');
        const captureFaceBtn = document.getElementById('captureFace');
        const saveFaceBtn = document.getElementById('saveFace');
        const skipFaceBtn = document.getElementById('skipFace');
        const statusMessage = document.getElementById('statusMessage');

        // Charger les modeles face-api
        async function loadModels() {
            try {
                statusMessage.textContent = 'Chargement des modeles de reconnaissance faciale...';
                statusMessage.className = 'status-message status-info';
                statusMessage.style.display = 'block';

                const MODEL_URL = './models/';

                await Promise.all([
                    faceapi.nets.tinyFaceDetector.loadFromUri(MODEL_URL),
                    faceapi.nets.faceLandmark68Net.loadFromUri(MODEL_URL),
                    faceapi.nets.faceRecognitionNet.loadFromUri(MODEL_URL)
                ]);

                modelsLoaded = true;
                statusMessage.textContent = 'Modeles charges avec succes !';
                statusMessage.className = 'status-message status-success';
                setTimeout(() => {
                    statusMessage.style.display = 'none';
                }, 2000);
            } catch (error) {
                console.error('Erreur lors du chargement des modeles:', error);
                statusMessage.textContent = 'Erreur lors du chargement des modeles. Veuillez rafraichir la page.';
                statusMessage.className = 'status-message status-error';
            }
        }

        // Demarrer la camera
        async function startCamera() {
            try {
                stream = await navigator.mediaDevices.getUserMedia({
                    video: {
                        width: 640,
                        height: 480,
                        facingMode: 'user'
                    }
                });
                video.srcObject = stream;
                startCameraBtn.disabled = true;
                captureFaceBtn.disabled = false;

                statusMessage.textContent = 'Camera activee. Positionnez votre visage au centre.';
                statusMessage.className = 'status-message status-info';
                statusMessage.style.display = 'block';

                // Detection continue du visage
                detectFace();
            } catch (error) {
                console.error('Erreur lors de l\'acces a la camera:', error);
                statusMessage.textContent = 'Impossible d\'acceder a la camera. Verifiez les permissions.';
                statusMessage.className = 'status-message status-error';
                statusMessage.style.display = 'block';
            }
        }

        // Detection continue du visage
        async function detectFace() {
            if (!modelsLoaded || !stream) return;

            const detection = await faceapi
                .detectSingleFace(video, new faceapi.TinyFaceDetectorOptions())
                .withFaceLandmarks()
                .withFaceDescriptor();

            if (detection) {
                const ctx = canvas.getContext('2d');
                canvas.width = video.videoWidth;
                canvas.height = video.videoHeight;
                ctx.clearRect(0, 0, canvas.width, canvas.height);

                const box = detection.detection.box;
                ctx.strokeStyle = '#00ff00';
                ctx.lineWidth = 2;
                ctx.strokeRect(box.x, box.y, box.width, box.height);
            }

            requestAnimationFrame(detectFace);
        }

        // Capturer le visage
        async function captureFace() {
            if (!modelsLoaded) {
                statusMessage.textContent = 'Les modeles ne sont pas encore charges. Veuillez patienter.';
                statusMessage.className = 'status-message status-error';
                statusMessage.style.display = 'block';
                return;
            }

            try {
                statusMessage.textContent = 'Capture en cours...';
                statusMessage.className = 'status-message status-info';
                statusMessage.style.display = 'block';

                const detection = await faceapi
                    .detectSingleFace(video, new faceapi.TinyFaceDetectorOptions())
                    .withFaceLandmarks()
                    .withFaceDescriptor();

                if (detection) {
                    faceEmbedding = Array.from(detection.descriptor);
                    saveFaceBtn.disabled = false;

                    statusMessage.textContent = 'Visage capture avec succes ! Vous pouvez maintenant enregistrer.';
                    statusMessage.className = 'status-message status-success';
                } else {
                    statusMessage.textContent = 'Aucun visage detecte. Veuillez reessayer.';
                    statusMessage.className = 'status-message status-error';
                }
            } catch (error) {
                console.error('Erreur lors de la capture:', error);
                statusMessage.textContent = 'Erreur lors de la capture. Veuillez reessayer.';
                statusMessage.className = 'status-message status-error';
            }
        }

        // Enregistrer le Face ID
        async function saveFace() {
            if (!faceEmbedding) {
                statusMessage.textContent = 'Veuillez d\'abord capturer votre visage.';
                statusMessage.className = 'status-message status-error';
                statusMessage.style.display = 'block';
                return;
            }

            try {
                statusMessage.textContent = 'Enregistrement en cours...';
                statusMessage.className = 'status-message status-info';
                statusMessage.style.display = 'block';
                saveFaceBtn.disabled = true;

                const response = await fetch('../controlleur/face_register_traitement.php', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({
                        face_embedding: faceEmbedding
                    })
                });

                const result = await response.json();

                if (result.success) {
                    statusMessage.textContent = 'Face ID enregistre avec succes !';
                    statusMessage.className = 'status-message status-success';

                    if (stream) {
                        stream.getTracks().forEach(track => track.stop());
                    }

                    setTimeout(() => {
                        window.location.href = 'login.php';
                    }, 2000);
                } else {
                    statusMessage.textContent = result.message || 'Erreur lors de l\'enregistrement.';
                    statusMessage.className = 'status-message status-error';
                    saveFaceBtn.disabled = false;
                }
            } catch (error) {
                console.error('Erreur:', error);
                statusMessage.textContent = 'Erreur lors de l\'enregistrement. Veuillez reessayer.';
                statusMessage.className = 'status-message status-error';
                saveFaceBtn.disabled = false;
            }
        }

        function skipFace() {
            if (stream) {
                stream.getTracks().forEach(track => track.stop());
            }
            window.location.href = 'login.php';
        }

        startCameraBtn.addEventListener('click', startCamera);
        captureFaceBtn.addEventListener('click', captureFace);
        saveFaceBtn.addEventListener('click', saveFace);
        skipFaceBtn.addEventListener('click', skipFace);

        loadModels();
    </script>
</body>
</html>
