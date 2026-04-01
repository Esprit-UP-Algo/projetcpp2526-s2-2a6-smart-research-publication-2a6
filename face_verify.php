<?php
session_start();
$error = $_SESSION['error'] ?? '';
unset($_SESSION['error']);
?>
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Connexion Face ID - Together4Peace</title>
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

        /* ── Video + overlay wrapper ─────────────────────────── */
        .video-wrapper {
            position: relative;
            display: block;
            width: 100%;
            max-width: 500px;
            margin: 0 auto;
            border-radius: 10px;
            overflow: hidden;
            background: #111;
            box-shadow: 0 4px 20px rgba(0,0,0,0.35);
        }
        #video {
            width: 100%;
            display: block;
            border-radius: 10px;
            background: #000;
        }
        /* Overlay canvas sits exactly over the video */
        #overlay {
            position: absolute;
            top: 0; left: 0;
            width: 100%; height: 100%;
            border-radius: 10px;
            pointer-events: none;
        }
        /* Hidden canvas used only for pixel sampling (brightness) */
        #canvas { display: none; }

        .face-detection-area {
            margin: 20px 0;
            text-align: center;
        }

        /* ── Status messages ─────────────────────────────────── */
        .status-message {
            padding: 12px 16px;
            border-radius: 8px;
            margin: 14px 0;
            text-align: center;
            font-weight: 500;
            font-size: 0.95em;
            transition: background-color 0.3s, color 0.3s, border-color 0.3s;
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
        .status-warning {
            background-color: #fff3cd;
            color: #856404;
            border: 1px solid #ffc107;
        }

        /* ── Buttons ─────────────────────────────────────────── */
        .btn-face {
            background-color: var(--color-primary);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 8px;
            font-size: 1.05em;
            cursor: pointer;
            margin: 8px 5px;
            transition: background-color 0.25s, box-shadow 0.25s;
            width: 100%;
            max-width: 300px;
        }
        .btn-face:hover:not(:disabled) {
            background-color: #001a3d;
        }
        .btn-face:disabled {
            background-color: #ccc;
            cursor: not-allowed;
            box-shadow: none;
        }
        /* Green pulsing state when face is ready */
        .btn-face-ready {
            background-color: #28a745 !important;
            animation: pulse-green 1.6s ease-in-out infinite;
        }
        @keyframes pulse-green {
            0%, 100% { box-shadow: 0 0 6px rgba(40,167,69,0.5); }
            50%       { box-shadow: 0 0 22px rgba(40,167,69,0.95); }
        }

        .btn-secondary {
            background-color: #6c757d;
        }
        .btn-secondary:hover:not(:disabled) {
            background-color: #5a6268;
        }

        /* ── Layout helpers ──────────────────────────────────── */
        .button-group {
            text-align: center;
            margin-top: 18px;
        }
        .back-link {
            text-align: center;
            margin-top: 20px;
        }
        .back-link a {
            color: var(--color-primary);
            text-decoration: none;
        }

        /* ── Progress bar for stability ──────────────────────── */
        .stability-bar-wrap {
            width: 100%;
            max-width: 500px;
            margin: 6px auto 0;
            background: #e9ecef;
            border-radius: 4px;
            height: 5px;
            overflow: hidden;
            display: none;
        }
        .stability-bar {
            height: 100%;
            width: 0%;
            border-radius: 4px;
            transition: width 0.25s, background-color 0.3s;
        }
    </style>
</head>
<body class="auth-page">
<div class="auth-container">
    <div class="auth-header">
        <img src="../logo.png" alt="Logo Together4Peace" class="auth-logo"
             style="height:60px; margin-bottom:20px;">
        <h2>Connexion Face ID</h2>
        <p>Positionnez votre visage dans le cadre pour vous connecter</p>
    </div>

    <?php if ($error): ?>
        <div class="status-message status-error">
            <i class="fas fa-exclamation-circle"></i>
            <?php echo htmlspecialchars($error); ?>
        </div>
    <?php endif; ?>

    <div class="face-detection-area">
        <div class="video-wrapper">
            <video id="video" autoplay playsinline muted></video>
            <canvas id="overlay"></canvas>
        </div>
        <canvas id="canvas"></canvas>

        <!-- Stability progress bar -->
        <div class="stability-bar-wrap" id="stabilityWrap">
            <div class="stability-bar" id="stabilityBar"></div>
        </div>

        <div id="statusMessage" class="status-message" style="display:none;"></div>
    </div>

    <div class="button-group">
        <button id="startCamera" class="btn-face">
            <i class="fas fa-video"></i> Activer la caméra
        </button>
        <button id="verifyFace" class="btn-face" disabled>
            <i class="fas fa-user-check"></i> Vérifier mon visage
        </button>
        <br>
        <a href="login.php" class="btn-face btn-secondary"
           style="display:inline-block; text-decoration:none; padding:10px 24px; font-size:0.95em;">
            <i class="fas fa-arrow-left"></i> Retour à la connexion classique
        </a>
    </div>
</div>

<script src="https://cdn.jsdelivr.net/npm/face-api.js@0.22.2/dist/face-api.min.js"></script>
<script>
// ═══════════════════════════════════════════════════════════════════
//  Constants
// ═══════════════════════════════════════════════════════════════════
const ANALYSIS_INTERVAL_MS  = 250;   // ms between analysis ticks
const REQUIRED_GOOD_FRAMES  = 6;     // stable frames before "ready" (~1.5 s)
const VERIFY_SAMPLE_COUNT   = 3;     // embeddings averaged per verification
const VERIFY_SAMPLE_DELAY   = 250;   // ms between capture samples

const QUALITY = {
    MIN_FACE_RATIO:    0.055,  // face area / frame area  — too far
    MAX_FACE_RATIO:    0.58,   // too close
    MAX_CENTER_OFFSET: 0.22,   // relative to frame dimension
    MAX_TILT_DEG:      15,     // eye-line tilt
    MIN_BRIGHTNESS:    50,     // avg luma — too dark
    MAX_BRIGHTNESS:    228,    // avg luma — too bright
    MIN_SCORE:         0.40,   // TinyFaceDetector confidence
};

// Detector options: fast for analysis loop, precise for capture
const OPTS_FAST    = () => new faceapi.TinyFaceDetectorOptions({ inputSize: 224, scoreThreshold: 0.38 });
const OPTS_PRECISE = () => new faceapi.TinyFaceDetectorOptions({ inputSize: 320, scoreThreshold: 0.45 });

// ═══════════════════════════════════════════════════════════════════
//  State
// ═══════════════════════════════════════════════════════════════════
let stream          = null;
let modelsLoaded    = false;
let isVerifying     = false;
let isAnalyzing     = false;   // mutex for analysis loop
let analysisTimer   = null;
let goodFrameCount  = 0;       // consecutive "all conditions met" frames
let isReady         = false;   // true when goodFrameCount >= REQUIRED_GOOD_FRAMES

// ═══════════════════════════════════════════════════════════════════
//  DOM
// ═══════════════════════════════════════════════════════════════════
const video          = document.getElementById('video');
const overlayCanvas  = document.getElementById('overlay');
const captureCanvas  = document.getElementById('canvas');
const startCameraBtn = document.getElementById('startCamera');
const verifyFaceBtn  = document.getElementById('verifyFace');
const statusMessage  = document.getElementById('statusMessage');
const stabilityWrap  = document.getElementById('stabilityWrap');
const stabilityBar   = document.getElementById('stabilityBar');

// ═══════════════════════════════════════════════════════════════════
//  Model loading
// ═══════════════════════════════════════════════════════════════════
async function loadModels() {
    try {
        updateStatus('⏳ Chargement des modèles de reconnaissance faciale...', 'info');
        const MODEL_URL = './models/';
        await Promise.all([
            faceapi.nets.tinyFaceDetector.loadFromUri(MODEL_URL),
            faceapi.nets.faceLandmark68Net.loadFromUri(MODEL_URL),
            faceapi.nets.faceRecognitionNet.loadFromUri(MODEL_URL),
        ]);
        modelsLoaded = true;
        updateStatus('✅ Prêt — cliquez sur "Activer la caméra"', 'good');
        setTimeout(() => { statusMessage.style.display = 'none'; }, 3500);
    } catch (e) {
        updateStatus('❌ Erreur de chargement des modèles. Rechargez la page.', 'error');
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Camera
// ═══════════════════════════════════════════════════════════════════
async function startCamera() {
    startCameraBtn.disabled = true;
    updateStatus('📷 Activation de la caméra...', 'info');
    try {
        stream = await navigator.mediaDevices.getUserMedia({
            video: { width: { ideal: 640 }, height: { ideal: 480 }, facingMode: 'user' }
        });
        video.srcObject = stream;
        await new Promise(resolve => { video.onloadedmetadata = resolve; });
        await video.play().catch(() => {});

        updateStatus('🔍 Analyse du visage en cours...', 'info');
        stabilityWrap.style.display = 'block';

        // Start periodic analysis (setInterval + mutex = no frame pile-up)
        analysisTimer = setInterval(analyzeFrame, ANALYSIS_INTERVAL_MS);
    } catch (e) {
        startCameraBtn.disabled = false;
        const msg = e.name === 'NotAllowedError'
            ? '❌ Accès à la caméra refusé. Autorisez la permission dans votre navigateur.'
            : '❌ Caméra inaccessible. Vérifiez qu\'elle n\'est pas utilisée ailleurs.';
        updateStatus(msg, 'error');
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Real-time face analysis loop
// ═══════════════════════════════════════════════════════════════════
async function analyzeFrame() {
    // Mutex — skip tick if previous analysis is still running
    if (!modelsLoaded || !stream || isVerifying || isAnalyzing) return;
    if (!video.readyState || video.readyState < 2 || video.videoWidth === 0) return;

    isAnalyzing = true;
    try {
        const W = video.videoWidth;
        const H = video.videoHeight;

        // Resize overlay canvas to match video resolution
        if (overlayCanvas.width !== W || overlayCanvas.height !== H) {
            overlayCanvas.width  = W;
            overlayCanvas.height = H;
        }
        const ctx = overlayCanvas.getContext('2d');
        ctx.clearRect(0, 0, W, H);

        const detection = await faceapi
            .detectSingleFace(video, OPTS_FAST())
            .withFaceLandmarks();

        if (!detection) {
            decGoodFrames();
            drawGuideOval(ctx, W, H, stateColor());
            updateStatus('👤 Aucun visage détecté — positionnez-vous face à la caméra', 'warn');
            setVerifyReady(false);
            updateStabilityBar();
            return;
        }

        const box   = detection.detection.box;
        const score = detection.detection.score;

        // ── Quality metrics ────────────────────────────────────
        const faceRatio = (box.width * box.height) / (W * H);
        const boxCx     = box.x + box.width  / 2;
        const boxCy     = box.y + box.height / 2;
        const offsetX   = (boxCx - W / 2) / W;   // signed: <0 = left
        const offsetY   = (boxCy - H / 2) / H;   // signed: <0 = top

        // Tilt: angle of the eye line
        const le   = detection.landmarks.getLeftEye();
        const re   = detection.landmarks.getRightEye();
        const leC  = avgPoint(le);
        const reC  = avgPoint(re);
        const tiltDeg = Math.abs(
            Math.atan2(reC.y - leC.y, reC.x - leC.x) * 180 / Math.PI
        );

        // Brightness: sample pixels in face bounding box
        const brightness = sampleBrightness(box, W, H);

        // ── Evaluate conditions ────────────────────────────────
        const issue = checkQuality(faceRatio, offsetX, offsetY, tiltDeg, brightness, score);

        if (issue) {
            decGoodFrames();
            drawGuideOval(ctx, W, H, stateColor());
            drawFaceCorners(ctx, box, stateColor());
            updateStatus(issue, 'warn');
            setVerifyReady(false);
        } else {
            incGoodFrames();
            const color = goodFrameCount >= REQUIRED_GOOD_FRAMES ? '#00e676' : '#ffd600';
            drawGuideOval(ctx, W, H, color);
            drawFaceCorners(ctx, box, color);

            if (goodFrameCount >= REQUIRED_GOOD_FRAMES) {
                updateStatus('✅ Visage correctement positionné — cliquez sur "Vérifier"', 'good');
                setVerifyReady(true);
            } else {
                const n = REQUIRED_GOOD_FRAMES - goodFrameCount;
                updateStatus(`⏳ Stabilisation... maintenez la position (${n} instant${n > 1 ? 's' : ''})`, 'info');
                setVerifyReady(false);
            }
        }

        updateStabilityBar();
    } catch (_) {
        // Absorb individual frame errors silently — next tick will retry
    } finally {
        isAnalyzing = false;
    }
}

// ── Quality checker — returns null if OK, else a message ──────────
function checkQuality(faceRatio, offsetX, offsetY, tiltDeg, brightness, score) {
    if (faceRatio < QUALITY.MIN_FACE_RATIO)
        return '📏 Trop loin — rapprochez-vous de la caméra';
    if (faceRatio > QUALITY.MAX_FACE_RATIO)
        return '📏 Trop près — reculez un peu';
    if (Math.abs(offsetX) > QUALITY.MAX_CENTER_OFFSET)
        return offsetX < 0
            ? '↔ Visage décalé à gauche — déplacez-vous vers le centre'
            : '↔ Visage décalé à droite — déplacez-vous vers le centre';
    if (Math.abs(offsetY) > QUALITY.MAX_CENTER_OFFSET)
        return offsetY < 0
            ? '↕ Visage trop haut — descendez légèrement'
            : '↕ Visage trop bas — remontez légèrement';
    if (tiltDeg > QUALITY.MAX_TILT_DEG)
        return '🔄 Visage incliné — redressez la tête';
    if (brightness < QUALITY.MIN_BRIGHTNESS)
        return '💡 Image trop sombre — améliorez l\'éclairage';
    if (brightness > QUALITY.MAX_BRIGHTNESS)
        return '☀️ Trop lumineux — évitez la lumière directe sur le visage';
    if (score < QUALITY.MIN_SCORE)
        return '👁️ Image floue ou visage partiellement visible — restez immobile';
    return null;
}

// ═══════════════════════════════════════════════════════════════════
//  Verification (multi-sample → averaged descriptor)
// ═══════════════════════════════════════════════════════════════════
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

function averageDescriptors(descriptors) {
    const len = descriptors[0].length;
    const avg = new Float32Array(len);
    for (const d of descriptors) for (let i = 0; i < len; i++) avg[i] += d[i];
    for (let i = 0; i < len; i++) avg[i] /= descriptors.length;
    return avg;
}

async function verifyFace() {
    if (!modelsLoaded) {
        updateStatus('⏳ Modèles pas encore prêts. Patientez quelques secondes.', 'warn');
        return;
    }
    if (!isReady) {
        updateStatus('⚠️ Positionnez correctement votre visage avant de vérifier.', 'warn');
        return;
    }
    if (isVerifying) return;

    isVerifying = true;
    verifyFaceBtn.disabled = true;
    verifyFaceBtn.classList.remove('btn-face-ready');

    try {
        updateStatus('📸 Capture en cours… restez immobile', 'info');

        // Collect VERIFY_SAMPLE_COUNT frames with precise detector
        const samples = [];
        for (let i = 0; i < VERIFY_SAMPLE_COUNT; i++) {
            if (i > 0) await sleep(VERIFY_SAMPLE_DELAY);
            const det = await faceapi
                .detectSingleFace(video, OPTS_PRECISE())
                .withFaceLandmarks()
                .withFaceDescriptor();
            if (det) samples.push(det);
        }

        if (samples.length === 0) {
            updateStatus('❌ Visage perdu pendant la capture. Repositionnez-vous.', 'error');
            isVerifying = false;
            goodFrameCount = 0;
            setVerifyReady(false);
            updateStabilityBar();
            return;
        }

        // Average all captured descriptors for robustness
        const embedding = samples.length === 1
            ? Array.from(samples[0].descriptor)
            : Array.from(averageDescriptors(samples.map(s => s.descriptor)));

        updateStatus('🔄 Vérification en cours…', 'info');

        const response = await fetch('../controlleur/face_verify_traitement.php', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ face_embedding: embedding }),
        });

        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const result = await response.json();

        if (result.success) {
            updateStatus('✅ Connexion réussie ! Redirection…', 'good');
            // Stop everything
            if (analysisTimer) { clearInterval(analysisTimer); analysisTimer = null; }
            const ctx = overlayCanvas.getContext('2d');
            ctx.clearRect(0, 0, overlayCanvas.width, overlayCanvas.height);
            drawGuideOval(ctx, overlayCanvas.width, overlayCanvas.height, '#00e676');
            if (stream) stream.getTracks().forEach(t => t.stop());
            setTimeout(() => { window.location.href = result.redirect || 'dashboard.php'; }, 1500);
        } else {
            updateStatus(result.message || '❌ Visage non reconnu. Réessayez.', 'error');
            isVerifying = false;
            // Penalize count slightly — need to re-stabilize before next attempt
            goodFrameCount = Math.max(0, goodFrameCount - 2);
            updateStabilityBar();
            if (goodFrameCount >= REQUIRED_GOOD_FRAMES) setVerifyReady(true);
        }
    } catch (e) {
        console.error('Verify error:', e);
        updateStatus('❌ Erreur réseau. Vérifiez votre connexion et réessayez.', 'error');
        isVerifying = false;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Canvas drawing helpers
// ═══════════════════════════════════════════════════════════════════
function drawGuideOval(ctx, W, H, color) {
    const cx = W / 2;
    const cy = H * 0.47;
    const rx = W * 0.235;
    const ry = H * 0.38;

    ctx.save();
    // Glow
    ctx.shadowColor = color;
    ctx.shadowBlur  = 10;
    ctx.strokeStyle = color;
    ctx.lineWidth   = 3;
    if (color !== '#00e676') ctx.setLineDash([10, 6]);

    ctx.beginPath();
    ctx.ellipse(cx, cy, rx, ry, 0, 0, Math.PI * 2);
    ctx.stroke();

    ctx.setLineDash([]);
    ctx.shadowBlur = 0;

    // Subtle inner fill
    ctx.fillStyle = color === '#00e676'
        ? 'rgba(0,230,118,0.05)'
        : 'rgba(255,255,255,0.02)';
    ctx.beginPath();
    ctx.ellipse(cx, cy, rx, ry, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
}

function drawFaceCorners(ctx, box, color) {
    const { x, y } = box;
    const w = box.width;
    const h = box.height;
    const s = Math.min(w, h) * 0.16;  // corner arm length

    ctx.save();
    ctx.strokeStyle = color;
    ctx.lineWidth   = 2.5;
    ctx.lineCap     = 'round';
    ctx.shadowColor = color;
    ctx.shadowBlur  = 5;

    ctx.beginPath();
    // Top-left
    ctx.moveTo(x,         y + s); ctx.lineTo(x,     y);     ctx.lineTo(x + s, y);
    // Top-right
    ctx.moveTo(x + w - s, y);     ctx.lineTo(x + w, y);     ctx.lineTo(x + w, y + s);
    // Bottom-right
    ctx.moveTo(x + w,     y + h - s); ctx.lineTo(x + w, y + h); ctx.lineTo(x + w - s, y + h);
    // Bottom-left
    ctx.moveTo(x + s,     y + h); ctx.lineTo(x,     y + h); ctx.lineTo(x, y + h - s);
    ctx.stroke();
    ctx.restore();
}

// ═══════════════════════════════════════════════════════════════════
//  Measurement helpers
// ═══════════════════════════════════════════════════════════════════
function avgPoint(pts) {
    return {
        x: pts.reduce((s, p) => s + p.x, 0) / pts.length,
        y: pts.reduce((s, p) => s + p.y, 0) / pts.length,
    };
}

function sampleBrightness(box, W, H) {
    try {
        captureCanvas.width  = W;
        captureCanvas.height = H;
        const ctx = captureCanvas.getContext('2d');
        ctx.drawImage(video, 0, 0, W, H);
        const x  = Math.max(0, Math.floor(box.x));
        const y  = Math.max(0, Math.floor(box.y));
        const bw = Math.min(Math.floor(box.width),  W - x);
        const bh = Math.min(Math.floor(box.height), H - y);
        if (bw <= 0 || bh <= 0) return 128;
        const data  = ctx.getImageData(x, y, bw, bh).data;
        const step  = 16;  // sample every 4th pixel (4 channels × 4 = 16) for speed
        let sum = 0, count = 0;
        for (let i = 0; i < data.length; i += step) {
            sum += data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
            count++;
        }
        return count > 0 ? sum / count : 128;
    } catch (_) {
        return 128;  // neutral fallback on SecurityError etc.
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Stability counter with gradual decay (tolerates brief bad frames)
// ═══════════════════════════════════════════════════════════════════
function incGoodFrames() {
    goodFrameCount = Math.min(goodFrameCount + 1, REQUIRED_GOOD_FRAMES + 3);
}
function decGoodFrames() {
    // Decay by 2 — recovers from a single bad frame within 1 good frame
    goodFrameCount = Math.max(0, goodFrameCount - 2);
    if (goodFrameCount < REQUIRED_GOOD_FRAMES && isReady) {
        isReady = false;
        setVerifyReady(false);
    }
}
function stateColor() {
    if (goodFrameCount >= REQUIRED_GOOD_FRAMES) return '#00e676';
    if (goodFrameCount >= 3)                    return '#ffd600';
    return '#ef5350';
}
function updateStabilityBar() {
    const pct = Math.min(100, (goodFrameCount / REQUIRED_GOOD_FRAMES) * 100);
    stabilityBar.style.width = pct + '%';
    stabilityBar.style.backgroundColor =
        pct >= 100 ? '#28a745' : pct >= 50 ? '#ffc107' : '#dc3545';
}

// ═══════════════════════════════════════════════════════════════════
//  UI helpers
// ═══════════════════════════════════════════════════════════════════
function updateStatus(msg, type) {
    statusMessage.textContent = msg;
    const map = { good: 'status-success', error: 'status-error',
                  warn: 'status-warning', info: 'status-info' };
    statusMessage.className = 'status-message ' + (map[type] || 'status-info');
    statusMessage.style.display = 'block';
}

function setVerifyReady(ready) {
    isReady = ready;
    if (ready && !isVerifying) {
        verifyFaceBtn.disabled = false;
        verifyFaceBtn.classList.add('btn-face-ready');
    } else {
        verifyFaceBtn.classList.remove('btn-face-ready');
        if (!isVerifying) verifyFaceBtn.disabled = true;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Event listeners
// ═══════════════════════════════════════════════════════════════════
startCameraBtn.addEventListener('click', startCamera);
verifyFaceBtn.addEventListener('click', verifyFace);

loadModels();
</script>
</body>
</html>
