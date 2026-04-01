<?php
// Entry point for Google OAuth without external PHP packages.
// Required env vars:
// - GOOGLE_CLIENT_ID
// - GOOGLE_CLIENT_SECRET
// Optional env var:
// - GOOGLE_REDIRECT_URI (defaults to current host + /google_callback.php)

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

function renderConfigError(string $message): void
{
    http_response_code(500);
    echo '<!DOCTYPE html><html lang="fr"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">';
    echo '<title>Google OAuth - Configuration manquante</title>';
    echo '<style>body{margin:0;font-family:Segoe UI,Arial,sans-serif;background:#f4f7f7;color:#24443f;display:grid;min-height:100vh;place-items:center;padding:20px}.card{max-width:760px;width:100%;background:#fff;border:1px solid #dbe8e5;border-radius:14px;padding:22px;box-shadow:0 12px 26px rgba(0,0,0,.08)}h1{margin:0 0 10px;color:#0a5f58}pre{background:#eef5f4;border:1px solid #d3e5e1;padding:10px;border-radius:8px;overflow:auto}</style>';
    echo '</head><body><div class="card">';
    echo '<h1>Configuration Google OAuth incomplète</h1>';
    echo '<p>' . htmlspecialchars($message, ENT_QUOTES, 'UTF-8') . '</p>';
    echo '<p>Définis ces variables avant de relancer le serveur PHP :</p>';
    echo '<pre>setx GOOGLE_CLIENT_ID "ton_client_id"
setx GOOGLE_CLIENT_SECRET "ton_client_secret"
setx GOOGLE_REDIRECT_URI "http://127.0.0.1:8000/google_callback.php"</pre>';
    echo '<p>Ou crée le fichier <strong>google_oauth_config.php</strong> avec :</p>';
    echo '<pre>&lt;?php
const GOOGLE_CLIENT_ID = "ton_client_id";
const GOOGLE_CLIENT_SECRET = "ton_client_secret";
const GOOGLE_REDIRECT_URI = "http://127.0.0.1:8000/google_callback.php";</pre>';
    echo '<p>Puis redémarre le terminal et le serveur PHP.</p>';
    echo '</div></body></html>';
}

$clientId = getConfigValue('GOOGLE_CLIENT_ID', 'GOOGLE_CLIENT_ID');
$clientSecret = getConfigValue('GOOGLE_CLIENT_SECRET', 'GOOGLE_CLIENT_SECRET');
$redirectUri = getConfigValue('GOOGLE_REDIRECT_URI', 'GOOGLE_REDIRECT_URI');

if ($redirectUri === '') {
    $redirectUri = currentBaseUrl() . '/google_callback.php';
}

if ($clientId === '' || $clientSecret === '') {
    renderConfigError('GOOGLE_CLIENT_ID ou GOOGLE_CLIENT_SECRET est vide.');
    exit;
}

$state = bin2hex(random_bytes(16));
$_SESSION['oauth2_state'] = $state;

$qtCallback = isset($_GET['qt_callback']) ? trim((string)$_GET['qt_callback']) : '';
if ($qtCallback !== '') {
    $_SESSION['oauth2_qt_callback'] = $qtCallback;
} else {
    unset($_SESSION['oauth2_qt_callback']);
}

$params = [
    'client_id' => $clientId,
    'redirect_uri' => $redirectUri,
    'response_type' => 'code',
    'scope' => 'openid email profile',
    'state' => $state,
    'access_type' => 'offline',
    'prompt' => 'consent'
];

$authUrl = 'https://accounts.google.com/o/oauth2/v2/auth?' . http_build_query($params);

header('Location: ' . $authUrl);
exit;
