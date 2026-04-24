-- Alter citation_count column to citations VARCHAR2
-- This script changes the citation_count column from NUMBER to VARCHAR2 to store citation text instead of count

ALTER TABLE "Publication" RENAME COLUMN "citation_count" TO "citations_old";

ALTER TABLE "Publication" ADD ("citations" VARCHAR2(4000) DEFAULT '');

-- Copy data: convert number to string if needed
UPDATE "Publication" SET "citations" = TO_CHAR(NVL("citations_old", 0)) WHERE "citations_old" IS NOT NULL;

-- Drop old column
ALTER TABLE "Publication" DROP COLUMN "citations_old";