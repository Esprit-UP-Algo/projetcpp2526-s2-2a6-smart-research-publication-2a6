-- Migration Oracle pour activer la connexion Google OAuth dans SmartVision
-- Table cible: "Employés"

-- 1) Ajouter les colonnes Google (id + photo)
ALTER TABLE "Employés" ADD (
    "GOOGLE_ID" VARCHAR2(255),
    "GOOGLE_PHOTO" VARCHAR2(500)
);
/

-- 2) Contrainte d'unicite sur GOOGLE_ID (autorise NULL)
ALTER TABLE "Employés" ADD CONSTRAINT "UQ_EMPLOYES_GOOGLE_ID" UNIQUE ("GOOGLE_ID");
/

-- 3) Verification rapide
-- SELECT "employee_id", "EMAIL", "GOOGLE_ID", "GOOGLE_PHOTO" FROM "Employés";
