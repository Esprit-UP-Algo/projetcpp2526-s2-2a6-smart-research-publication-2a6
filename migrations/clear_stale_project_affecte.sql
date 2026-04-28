-- Backup recommendation (optional): create a backup table
-- CREATE TABLE "Employés_backup" AS SELECT * FROM "Employés";

-- Preview affected rows
SELECT "employee_id", "PROJET_AFFECTE" FROM "Employés" e
WHERE TRIM(NVL("PROJET_AFFECTE", '')) IS NOT NULL
  AND NOT EXISTS (SELECT 1 FROM "projet" p WHERE p."nom_du_projet" = e."PROJET_AFFECTE");

-- When you're satisfied with the preview, run the update to clear stale assignments
UPDATE "Employés" e
SET "PROJET_AFFECTE" = NULL
WHERE TRIM(NVL("PROJET_AFFECTE", '')) IS NOT NULL
  AND NOT EXISTS (SELECT 1 FROM "projet" p WHERE p."nom_du_projet" = e."PROJET_AFFECTE");

COMMIT;