#include "CitySuggestionModel.h"

#include <utility>

CitySuggestionModel::CitySuggestionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CitySuggestionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant CitySuggestionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case NameRole:
    case Qt::DisplayRole:
        return entry.name;
    case DetailRole:
        return entry.detail;
    case CountryRole:
        return entry.country;
    case AdminAreaRole:
        return entry.adminArea;
    case LatitudeRole:
        return entry.latitude;
    case LongitudeRole:
        return entry.longitude;
    case SourceRole:
        return entry.source;
    case AdcodeRole:
        return entry.adcode;
    case LevelRole:
        return entry.level;
    default:
        return {};
    }
}

QHash<int, QByteArray> CitySuggestionModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {DetailRole, "detail"},
        {CountryRole, "country"},
        {AdminAreaRole, "adminArea"},
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
        {SourceRole, "source"},
        {AdcodeRole, "adcode"},
        {LevelRole, "level"}
    };
}

void CitySuggestionModel::setEntries(QVector<Entry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
}

void CitySuggestionModel::clear()
{
    if (m_entries.isEmpty())
        return;

    beginResetModel();
    m_entries.clear();
    endResetModel();
}

CitySuggestionModel::Entry CitySuggestionModel::entryAt(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    return m_entries.at(row);
}
