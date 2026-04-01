-- Migration Face ID (MySQL 5.7+)
-- Adaptez le nom de la table si ce n'est pas `users`.

ALTER TABLE users
    ADD COLUMN face_embedding JSON NULL,
    ADD COLUMN face_enabled TINYINT(1) NOT NULL DEFAULT 0,
    ADD COLUMN face_registered_at DATETIME NULL;

-- Optionnel: activer Face ID pour les utilisateurs qui ont deja un embedding
UPDATE users
SET face_enabled = 1,
    face_registered_at = COALESCE(face_registered_at, NOW())
WHERE face_embedding IS NOT NULL;
