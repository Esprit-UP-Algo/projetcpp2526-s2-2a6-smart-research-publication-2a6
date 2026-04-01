-- Add optional metrics columns for Publication ranking and CV export.
-- Safe to run multiple times.

BEGIN
    EXECUTE IMMEDIATE 'ALTER TABLE "Publication" ADD ("impact_factor" NUMBER(8,2) DEFAULT 0)';
EXCEPTION
    WHEN OTHERS THEN
        IF SQLCODE != -01430 THEN
            RAISE;
        END IF;
END;
/

BEGIN
    EXECUTE IMMEDIATE 'ALTER TABLE "Publication" ADD ("citation_count" NUMBER DEFAULT 0)';
EXCEPTION
    WHEN OTHERS THEN
        IF SQLCODE != -01430 THEN
            RAISE;
        END IF;
END;
/

UPDATE "Publication"
SET "impact_factor" = NVL("impact_factor", 0),
    "citation_count" = NVL("citation_count", 0);

COMMIT;
