-- 1. Table EMPLOYES
CREATE TABLE Employes (
    employee_id    NUMBER PRIMARY KEY,
    CIN            VARCHAR2(20) UNIQUE NOT NULL,
    nom            VARCHAR2(100) NOT NULL,
    prenom         VARCHAR2(100) NOT NULL,
    role           VARCHAR2(50),
    specialization VARCHAR2(100)
);

-- 2. Table PROJET
CREATE TABLE Projet (
    Id_projet                    NUMBER PRIMARY KEY,
    nom_du_projet                VARCHAR2(200) NOT NULL,
    domaine_de_recherche         VARCHAR2(200),
    date_de_debut                DATE,
    date_de_fin                  DATE,
    budget                       NUMBER(15,2),
    statut                       VARCHAR2(50),
    source_de_financement        VARCHAR2(200),
    numero_d_approbation_ethique VARCHAR2(100),
    nombre_de_publications       NUMBER DEFAULT 0,
    employee_id                  NUMBER,
    CONSTRAINT fk_projet_emp FOREIGN KEY (employee_id)
        REFERENCES Employes(employee_id)
);

-- 3. Table EQUIPEMENT
CREATE TABLE Equipement (
    equipement_id              NUMBER PRIMARY KEY,
    nom_equipement             VARCHAR2(200) NOT NULL,
    fabricant                  VARCHAR2(200),
    numero_de_modele           VARCHAR2(100),
    date_achat                 DATE,
    date_derniere_maintenance  DATE,
    date_prochaine_maintenance DATE,
    statut                     VARCHAR2(50),
    localisation               VARCHAR2(200),
    date_limite_calibration    DATE
);

-- 4. Table EXPERIENCE
-- Relation : Projet CONDUIRE Experience (1-N)
-- Relation : Experience UTILISE Equipement (1-N)
CREATE TABLE Experience (
    Id_exp         NUMBER PRIMARY KEY,
    Titre          VARCHAR2(200) NOT NULL,
    Hypothese      VARCHAR2(500),
    Date_Debut     DATE,
    Date_fin       DATE,
    Status         VARCHAR2(50),
    Id_projet      NUMBER NOT NULL,
    equipement_id  NUMBER,
    CONSTRAINT fk_exp_projet FOREIGN KEY (Id_projet)
        REFERENCES Projet(Id_projet),
    CONSTRAINT fk_exp_equip FOREIGN KEY (equipement_id)
        REFERENCES Equipement(equipement_id)
);

-- 5. Table BIOSAMPLE
-- Relation : Projet CONTENIR BioSample (1-N)
CREATE TABLE BioSample (
    ID_echantillon          NUMBER PRIMARY KEY,
    Reference_echantillon   VARCHAR2(100) UNIQUE NOT NULL,
    Type_echantillon        VARCHAR2(100),
    Organisme_source        VARCHAR2(200),
    Date_de_collecte        DATE,
    Emplacement_de_stockage VARCHAR2(200),
    Temperature_de_stockage NUMBER(5,2),
    Quantite_restante       NUMBER(10,3),
    Date_expiration         DATE,
    Niveau_de_dangerosite   VARCHAR2(50),
    Id_projet               NUMBER NOT NULL,
    CONSTRAINT fk_bio_projet FOREIGN KEY (Id_projet)
        REFERENCES Projet(Id_projet)
);

-- 6. Table PUBLICATION
-- Relation : Projet POSSEDER Publication (1-N)
-- Relation : Employes ECRIRE Publication (1-N)
CREATE TABLE Publication (
    id          NUMBER PRIMARY KEY,
    titre       VARCHAR2(300) NOT NULL,
    journal     VARCHAR2(200),
    annee       NUMBER(4),
    DOI         VARCHAR2(100) UNIQUE,
    status      VARCHAR2(50),
    abstract    CLOB,
    Id_projet   NUMBER NOT NULL,
    employee_id NUMBER NOT NULL,
    CONSTRAINT fk_pub_projet FOREIGN KEY (Id_projet)
        REFERENCES Projet(Id_projet),
    CONSTRAINT fk_pub_emp FOREIGN KEY (employee_id)
        REFERENCES Employes(employee_id)
);