--------------------------------------------------------------------------------
-- SMARTVISION - VERSION COMPATIBLE ORACLE
-- Sans IDENTITY
-- Sans INDEX
-- Sans INSERT
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- DROP TABLES
--------------------------------------------------------------------------------

BEGIN EXECUTE IMMEDIATE 'DROP TABLE "Utiliser" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/
BEGIN EXECUTE IMMEDIATE 'DROP TABLE "Ecrire" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/
BEGIN EXECUTE IMMEDIATE 'DROP TABLE "Associer" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/
BEGIN EXECUTE IMMEDIATE 'DROP TABLE "BioSample" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/
BEGIN EXECUTE IMMEDIATE 'DROP TABLE "Équipement" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/
BEGIN EXECUTE IMMEDIATE 'DROP TABLE "Publication" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/
BEGIN EXECUTE IMMEDIATE 'DROP TABLE "Expérience" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/
BEGIN EXECUTE IMMEDIATE 'DROP TABLE "projet" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/
BEGIN EXECUTE IMMEDIATE 'DROP TABLE "Employés" CASCADE CONSTRAINTS'; EXCEPTION WHEN OTHERS THEN NULL; END;
/

--------------------------------------------------------------------------------
-- TABLE "Employés"
--------------------------------------------------------------------------------

CREATE TABLE "Employés" (
    "employee_id"     NUMBER NOT NULL,
    "CIN"             VARCHAR2(20) NOT NULL,
    "nom"             VARCHAR2(100) NOT NULL,
    "prenom"          VARCHAR2(100) NOT NULL,
    "specialization"  VARCHAR2(150),
    "EMAIL"           VARCHAR2(120),
    "USER_PASSWORD"   VARCHAR2(255) NOT NULL,
    "FULL_NAME"       VARCHAR2(120),
    "ROLE"            VARCHAR2(30) DEFAULT 'USER' NOT NULL,
    "ACTIVE"          CHAR(1) DEFAULT 'O' NOT NULL,
    "FACE_ID"         BLOB,

    CONSTRAINT PK_EMPLOYES PRIMARY KEY ("employee_id"),
    CONSTRAINT UQ_EMPLOYES_CIN UNIQUE ("CIN"),
    CONSTRAINT UQ_EMPLOYES_EMAIL UNIQUE ("EMAIL"),
    CONSTRAINT CK_EMPLOYES_ACTIVE CHECK ("ACTIVE" IN ('O','N'))
);
/

--------------------------------------------------------------------------------
-- TABLE "projet"
--------------------------------------------------------------------------------

CREATE TABLE "projet" (
    "Id_projet"                     NUMBER NOT NULL,
    "nom_du_projet"                 VARCHAR2(150) NOT NULL,
    "domaine_de_recherche"          VARCHAR2(150),
    "date_de_début"                 DATE,
    "date_de_fin"                   DATE,
    "budget"                        NUMBER(14,2),
    "statut"                        VARCHAR2(50),
    "source_de_financement"         VARCHAR2(150),
    "numéro_d_approbation_éthique"  VARCHAR2(100),
    "nombre_de_publications"        NUMBER DEFAULT 0,

    CONSTRAINT PK_PROJET PRIMARY KEY ("Id_projet")
);
/

--------------------------------------------------------------------------------
-- TABLE "Expérience"
--------------------------------------------------------------------------------

CREATE TABLE "Expérience" (
    "Id_exp"       NUMBER NOT NULL,
    "Titre"        VARCHAR2(150) NOT NULL,
    "Hypothese"    VARCHAR2(500),
    "Date_Debut"   DATE,
    "Date_fin"     DATE,
    "Status"       VARCHAR2(50),
    "Id_projet"    NUMBER NOT NULL,

    CONSTRAINT PK_EXPERIENCE PRIMARY KEY ("Id_exp"),
    CONSTRAINT FK_EXPERIENCE_PROJET FOREIGN KEY ("Id_projet")
        REFERENCES "projet" ("Id_projet")
);
/

--------------------------------------------------------------------------------
-- TABLE "Publication"
--------------------------------------------------------------------------------

CREATE TABLE "Publication" (
    "id_publication" NUMBER NOT NULL,
    "titre"          VARCHAR2(250) NOT NULL,
    "journal"        VARCHAR2(150),
    "année"          NUMBER(4),
    "DOI"            VARCHAR2(120),
    "status"         VARCHAR2(50),
    "abstract"       CLOB,

    CONSTRAINT PK_PUBLICATION PRIMARY KEY ("id_publication")
);
/

--------------------------------------------------------------------------------
-- TABLE "Équipement"
--------------------------------------------------------------------------------

CREATE TABLE "Équipement" (
    "equipement_id"              NUMBER NOT NULL,
    "nom_equipement"             VARCHAR2(150) NOT NULL,
    "fabricant"                  VARCHAR2(100),
    "numéro_de_modèle"           VARCHAR2(100),
    "date_d_achat"               DATE,
    "date_dernière_maintenance"  DATE,
    "date_prochaine_maintenance" DATE,
    "statut"                     VARCHAR2(50),
    "localisation"               VARCHAR2(150),
    "date_limite_calibration"    DATE,
    "disponibilité"              VARCHAR2(50),

    CONSTRAINT PK_EQUIPEMENT PRIMARY KEY ("equipement_id")
);
/

--------------------------------------------------------------------------------
-- TABLE "BioSample"
--------------------------------------------------------------------------------

CREATE TABLE "BioSample" (
    "ID_de_léchantillon"         NUMBER NOT NULL,
    "Reference_de_léchantillon"  VARCHAR2(100) NOT NULL,
    "Type_déchantillon"          VARCHAR2(100),
    "Organisme_source"           VARCHAR2(100),
    "Date_de_collecte"           DATE,
    "Emplacement_de_stockage"    VARCHAR2(150),
    "Température_de_stockage"    VARCHAR2(50),
    "Quantité_restante"          NUMBER(12,2),
    "Date_dexpiration"           DATE,
    "Niveau_de_dangerosité"      VARCHAR2(50),
    "Id_projet"                  NUMBER NOT NULL,

    CONSTRAINT PK_BIOSAMPLE PRIMARY KEY ("ID_de_léchantillon"),
    CONSTRAINT FK_BIOSAMPLE_PROJET FOREIGN KEY ("Id_projet")
        REFERENCES "projet" ("Id_projet")
);
/

--------------------------------------------------------------------------------
-- TABLE RELATIONNELLE "Associer"
--------------------------------------------------------------------------------

CREATE TABLE "Associer" (
    "employee_id"      NUMBER NOT NULL,
    "Id_projet"        NUMBER NOT NULL,
        CONSTRAINT PK_ASSOCIER PRIMARY KEY ("employee_id", "Id_projet"),
    CONSTRAINT FK_ASSOCIER_EMP FOREIGN KEY ("employee_id")
        REFERENCES "Employés" ("employee_id"),
    CONSTRAINT FK_ASSOCIER_PROJET FOREIGN KEY ("Id_projet")
        REFERENCES "projet" ("Id_projet")
);
/

--------------------------------------------------------------------------------
-- TABLE RELATIONNELLE "Ecrire"
--------------------------------------------------------------------------------

CREATE TABLE "Ecrire" (
    "employee_id"      NUMBER NOT NULL,
    "id_publication"   NUMBER NOT NULL,
   
    CONSTRAINT PK_ECRIRE PRIMARY KEY ("employee_id", "id_publication"),
    CONSTRAINT FK_ECRIRE_EMP FOREIGN KEY ("employee_id")
        REFERENCES "Employés" ("employee_id"),
    CONSTRAINT FK_ECRIRE_PUB FOREIGN KEY ("id_publication")
        REFERENCES "Publication" ("id_publication")
);
/

--------------------------------------------------------------------------------
-- TABLE RELATIONNELLE "Utiliser"
--------------------------------------------------------------------------------

CREATE TABLE "Utiliser" (
    "Id_exp"           NUMBER NOT NULL,
    "equipement_id"    NUMBER NOT NULL,
   
    CONSTRAINT PK_UTILISER PRIMARY KEY ("Id_exp", "equipement_id"),
    CONSTRAINT FK_UTILISER_EXP FOREIGN KEY ("Id_exp")
        REFERENCES "Expérience" ("Id_exp"),
    CONSTRAINT FK_UTILISER_EQ FOREIGN KEY ("equipement_id")
        REFERENCES "Équipement" ("equipement_id")
);
/