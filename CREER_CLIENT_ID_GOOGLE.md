# Guide : Creer un Client ID OAuth Google

## Probleme identifie

Le Client ID actuel (`6LeTwCcsAAAAAGN_YyOEltwUm2CQ8r4GWUQG7JSP`) est une cle reCAPTCHA, pas un Client ID OAuth. C'est pourquoi vous obtenez l'erreur "invalid_client".

## Solution : Creer un vrai Client ID OAuth 2.0

### Etape 1 : Acceder a Google Cloud Console

1. Allez sur [Google Cloud Console](https://console.cloud.google.com/)
2. Connectez-vous avec votre compte Google (kacemrayen059@gmail.com)

### Etape 2 : Creer ou selectionner un projet

1. En haut de la page, cliquez sur le selecteur de projet
2. Cliquez sur "NOUVEAU PROJET" (ou selectionnez un projet existant)
3. Nom du projet : `Together4Peace` (ou votre choix)
4. Cliquez sur "CREER"

### Etape 3 : Activer les APIs necessaires

1. Dans le menu de gauche, allez dans "APIs et services" > "Bibliotheque"
2. Recherchez "Google+ API" ou "Google Identity API"
3. Cliquez dessus et cliquez sur "ACTIVER"
4. (Optionnel) Activez aussi "People API" pour plus d'informations utilisateur

### Etape 4 : Configurer l'ecran de consentement OAuth

1. Allez dans "APIs et services" > "Ecran de consentement OAuth"
2. Selectionnez "Externe" (pour les tests) et cliquez sur "CREER"
3. Remplissez les informations :
   - Nom de l'application : Together4Peace
   - Adresse e-mail de support utilisateur : votre email
   - Adresse e-mail du developpeur : votre email
4. Cliquez sur "ENREGISTRER ET CONTINUER"
5. Dans "Scopes", cliquez sur "ENREGISTRER ET CONTINUER" (les scopes par defaut suffisent)
6. Dans "Utilisateurs de test", ajoutez votre email si necessaire, puis "ENREGISTRER ET CONTINUER"
7. Cliquez sur "RETOUR AU TABLEAU DE BORD"

### Etape 5 : Creer l'ID client OAuth 2.0

1. Allez dans "APIs et services" > "Identifiants"
2. Cliquez sur "CREER DES IDENTIFIANTS" en haut
3. Selectionnez "ID client OAuth 2.0"

### Etape 6 : Configurer l'ID client

1. Type d'application : Selectionnez "Application Web"
2. Nom : `Together4Peace Web Client` (ou votre choix)
3. URIs de redirection autorises :
   - Cliquez sur "+ AJOUTER UN URI"
   - Ajoutez : `http://localhost/dhia/controlleur/google_callback.php`
   - IMPORTANT : L'URI doit correspondre EXACTEMENT (pas d'espace, meme protocole)
4. Cliquez sur "CREER"

### Etape 7 : Copier les identifiants

Apres la creation, vous verrez une fenetre avec :
- Votre ID client : Format `xxxxx-xxxxx.apps.googleusercontent.com`
- Votre secret client : Format `GOCSPX-xxxxx...`

Copiez-les immediatement car le secret ne sera plus affiche.

### Etape 8 : Mettre a jour la configuration

1. A la racine du projet, copiez `google_oauth_config.example.php` vers `google_oauth_config.php`
2. Remplacez les valeurs :

```php
const GOOGLE_CLIENT_ID = 'VOTRE_NOUVEAU_CLIENT_ID.apps.googleusercontent.com';
const GOOGLE_CLIENT_SECRET = 'VOTRE_NOUVEAU_CLIENT_SECRET';
const GOOGLE_REDIRECT_URI = 'http://127.0.0.1:8000/google_callback.php';
```

3. Enregistrez le fichier

### Etape 9 : Verifier l'URI de redirection

1. Accedez a : `http://localhost/dhia/controlleur/test_google_redirect.php`
2. Copiez l'URI affichee
3. Retournez dans Google Cloud Console > Identifiants > Votre ID client
4. Verifiez que l'URI dans "URIs de redirection autorises" correspond EXACTEMENT
5. Si ce n'est pas le cas, modifiez-la pour qu'elle corresponde

### Etape 10 : Tester

1. Allez sur `http://localhost/dhia/views/login.php`
2. Cliquez sur "Se connecter avec Google"
3. Autorisez l'application
4. Vous devriez etre redirige vers le dashboard

## Format attendu

- Client ID OAuth : `123456789-abcdefghijklmnop.apps.googleusercontent.com`
- Client Secret : `GOCSPX-xxxxxxxxxxxxxxxxxxxxx`
- Cle reCAPTCHA (ne fonctionne pas) : `6LeTwCcsAAAAAGN_YyOEltwUm2CQ8r4GWUQG7JSP`

## Verification

Pour verifier que vous avez le bon type d'identifiant :
- Client ID OAuth : se termine par `.apps.googleusercontent.com`
- Cle reCAPTCHA : commence souvent par `6L` et ne se termine pas par `.apps.googleusercontent.com`

## Notes importantes

1. Le Client ID et le Client Secret doivent provenir du meme ID client OAuth 2.0
2. L'URI de redirection doit correspondre exactement (caractere par caractere)
3. Pour la production, vous devrez ajouter une autre URI avec `https://`
4. Gardez vos identifiants secrets et ne les partagez jamais publiquement
