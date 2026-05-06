#ifndef PUBLICATION_H
#define PUBLICATION_H

#include <QString>
#include <QSqlQueryModel>

class Publication
{
public:
    Publication();
    Publication(int id,
                const QString& titre,
                const QString& journal,
                int annee,
                const QString& doi,
                const QString& status,
                const QString& abstractText,
                int idProjet,
                int employeeId,
                double impactFactor = 0.0,
                const QString& citations = QString());

    int id() const;
    QString titre() const;
    QString journal() const;
    int annee() const;
    QString doi() const;
    QString status() const;
    QString abstractText() const;
    int idProjet() const;
    int employeeId() const;
    double impactFactor() const;
    QString citations() const;

    void setId(int id);
    void setTitre(const QString& titre);
    void setJournal(const QString& journal);
    void setAnnee(int annee);
    void setDoi(const QString& doi);
    void setStatus(const QString& status);
    void setAbstractText(const QString& abstractText);
    void setIdProjet(int idProjet);
    void setEmployeeId(int employeeId);
    void setImpactFactor(double impactFactor);
    void setCitations(const QString& citations);

    bool create(QString* errorMessage = nullptr) const;
    bool update(QString* errorMessage = nullptr) const;

    static bool remove(int id, QString* errorMessage = nullptr);
    static bool readById(int id, Publication& outPublication, QString* errorMessage = nullptr);
    static QSqlQueryModel* readAll(QObject* parent = nullptr, QString* errorMessage = nullptr);

private:
    int m_id;
    QString m_titre;
    QString m_journal;
    int m_annee;
    QString m_doi;
    QString m_status;
    QString m_abstractText;
    int m_idProjet;
    int m_employeeId;
    double m_impactFactor;
    QString m_citations;
};

#endif // PUBLICATION_H
