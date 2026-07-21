#include "MapSearchResultModel.h"

#include <QtGlobal>
#include <utility>

MapSearchResultModel::MapSearchResultModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MapSearchResultModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_results.size();
}

QVariant MapSearchResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_results.size())
        return {};

    const MapPlaceResult &place = m_results.at(index.row());
    switch (role) {
    case PlaceIdRole: return place.id;
    case NameRole:
    case Qt::DisplayRole: return place.name;
    case DistrictRole: return place.district;
    case AddressRole: return place.address;
    case LongitudeRole: return place.longitude;
    case LatitudeRole: return place.latitude;
    case DistanceMetersRole: return place.distanceMeters;
    case DistanceTextRole:
        if (place.distanceMeters < 1000.0)
            return QStringLiteral("%1 m").arg(qRound(place.distanceMeters));
        return QStringLiteral("%1 km").arg(place.distanceMeters / 1000.0, 0, 'f', 1);
    case RelevanceScoreRole: return place.relevanceScore;
    default: return {};
    }
}

QHash<int, QByteArray> MapSearchResultModel::roleNames() const
{
    return {
        {PlaceIdRole, "placeId"},
        {NameRole, "name"},
        {DistrictRole, "district"},
        {AddressRole, "address"},
        {LongitudeRole, "longitude"},
        {LatitudeRole, "latitude"},
        {DistanceMetersRole, "distanceMeters"},
        {DistanceTextRole, "distanceText"},
        {RelevanceScoreRole, "relevanceScore"}
    };
}

void MapSearchResultModel::setResults(QVector<MapPlaceResult> results)
{
    beginResetModel();
    m_results = std::move(results);
    endResetModel();
}

void MapSearchResultModel::clear()
{
    if (m_results.isEmpty())
        return;
    beginResetModel();
    m_results.clear();
    endResetModel();
}

const MapPlaceResult *MapSearchResultModel::placeAt(int row) const
{
    if (row < 0 || row >= m_results.size())
        return nullptr;
    return &m_results.at(row);
}
