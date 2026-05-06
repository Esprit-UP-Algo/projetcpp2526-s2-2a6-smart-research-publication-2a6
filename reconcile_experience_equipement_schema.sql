-- Oracle schema reconciliation for SmartVision Experience/Equipement modules
-- Safe to run on existing databases.

DECLARE
    v_count NUMBER := 0;
    v_exp_table USER_TABLES.TABLE_NAME%TYPE;
    v_eq_table  USER_TABLES.TABLE_NAME%TYPE;

    FUNCTION qname(p_name IN VARCHAR2) RETURN VARCHAR2 IS
    BEGIN
        RETURN '"' || REPLACE(p_name, '"', '""') || '"';
    END;
BEGIN
    -- Find the experience table by its signature columns (encoding-safe).
    BEGIN
        SELECT table_name
          INTO v_exp_table
          FROM (
                SELECT t.table_name
                  FROM USER_TABLES t
                 WHERE EXISTS (
                           SELECT 1 FROM USER_TAB_COLUMNS c
                            WHERE c.table_name = t.table_name
                              AND UPPER(c.column_name) = UPPER('Id_exp'))
                   AND EXISTS (
                           SELECT 1 FROM USER_TAB_COLUMNS c
                            WHERE c.table_name = t.table_name
                              AND UPPER(c.column_name) = UPPER('Titre'))
                   AND EXISTS (
                           SELECT 1 FROM USER_TAB_COLUMNS c
                            WHERE c.table_name = t.table_name
                              AND UPPER(c.column_name) = UPPER('Id_projet'))
                 ORDER BY t.table_name
               )
         WHERE ROWNUM = 1;
    EXCEPTION
        WHEN NO_DATA_FOUND THEN
            v_exp_table := NULL;
    END;

    IF v_exp_table IS NOT NULL THEN
        SELECT COUNT(*) INTO v_count
          FROM USER_TAB_COLUMNS
         WHERE table_name = v_exp_table
           AND UPPER(column_name) = UPPER('Type_Experience');
        IF v_count = 0 THEN
            EXECUTE IMMEDIATE 'ALTER TABLE ' || qname(v_exp_table) || ' ADD ("Type_Experience" VARCHAR2(150))';
        END IF;

        SELECT COUNT(*) INTO v_count
          FROM USER_TAB_COLUMNS
         WHERE table_name = v_exp_table
           AND UPPER(column_name) = UPPER('Disponibilite_Equipement');
        IF v_count = 0 THEN
            EXECUTE IMMEDIATE 'ALTER TABLE ' || qname(v_exp_table) || ' ADD ("Disponibilite_Equipement" VARCHAR2(50))';
        END IF;

        SELECT COUNT(*) INTO v_count
          FROM USER_TAB_COLUMNS
         WHERE table_name = v_exp_table
           AND UPPER(column_name) = UPPER('Resultat');
        IF v_count = 0 THEN
            EXECUTE IMMEDIATE 'ALTER TABLE ' || qname(v_exp_table) || ' ADD ("Resultat" VARCHAR2(500))';
        END IF;
    END IF;

    -- Find the equipment table by signature columns.
    BEGIN
        SELECT table_name
          INTO v_eq_table
          FROM (
                SELECT t.table_name
                  FROM USER_TABLES t
                 WHERE EXISTS (
                           SELECT 1 FROM USER_TAB_COLUMNS c
                            WHERE c.table_name = t.table_name
                              AND UPPER(c.column_name) = UPPER('equipement_id'))
                   AND EXISTS (
                           SELECT 1 FROM USER_TAB_COLUMNS c
                            WHERE c.table_name = t.table_name
                              AND UPPER(c.column_name) = UPPER('nom_equipement'))
                   AND EXISTS (
                           SELECT 1 FROM USER_TAB_COLUMNS c
                            WHERE c.table_name = t.table_name
                              AND UPPER(c.column_name) = UPPER('Id_exp'))
                 ORDER BY t.table_name
               )
         WHERE ROWNUM = 1;
    EXCEPTION
        WHEN NO_DATA_FOUND THEN
            v_eq_table := NULL;
    END;

    IF v_eq_table IS NOT NULL THEN
        SELECT COUNT(*) INTO v_count
          FROM USER_TAB_COLUMNS
         WHERE table_name = v_eq_table
           AND UPPER(column_name) = UPPER('Id_exp')
           AND nullable = 'N';

        IF v_count > 0 THEN
            EXECUTE IMMEDIATE 'ALTER TABLE ' || qname(v_eq_table) || ' MODIFY ("Id_exp" NULL)';
        END IF;
    END IF;
END;
/
