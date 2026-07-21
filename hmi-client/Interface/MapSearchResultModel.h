#ifndef MAPSEARCHRESULTMODEL_H
#define MAPSEARCHRESULTMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct MapPlaceResult
{
    QString id;
    QString name;
    QString district;
    QString address;
    double longitude = 0.0;
    double latitude = 0.0;
    double distanceMeters = 0.0;
    int relevanceScore = 0;
};

class MapSearchResultModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        PlaceIdRole = Qt::UserRole + 1,
        NameRole,
        DistrictRole,
        AddressRole,
        LongitudeRole,
        LatitudeRole,
        DistanceMetersRole,
        DistanceTextRole,
        RelevanceScoreRole
    };
    Q_ENUM(Role)

    explicit MapSearchResultModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setResults(QVector<MapPlaceResult> results);
    void clear();
    const MapPlaceResult *placeAt(int row) const;

private:
    QVector<MapPlaceResult> m_results;
};

#endif
