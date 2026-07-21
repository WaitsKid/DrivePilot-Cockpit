#ifndef CITYSUGGESTIONMODEL_H
#define CITYSUGGESTIONMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class CitySuggestionModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DetailRole,
        CountryRole,
        AdminAreaRole,
        LatitudeRole,
        LongitudeRole,
        SourceRole,
        AdcodeRole,
        LevelRole
    };
    Q_ENUM(Role)

    struct Entry {
        QString name;
        QString detail;
        QString country;
        QString adminArea;
        double latitude = 0.0;
        double longitude = 0.0;
        QString source;
        QString adcode;
        QString level;
    };

    explicit CitySuggestionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<Entry> entries);
    void clear();
    Entry entryAt(int row) const;

private:
    QVector<Entry> m_entries;
};

#endif
