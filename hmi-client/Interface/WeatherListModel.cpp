#include "WeatherListModel.h"

#include <utility>

WeatherListModel::WeatherListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int WeatherListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant WeatherListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case TimeTextRole:
        return entry.timeText;
    case DateTextRole:
        return entry.dateText;
    case WeekdayTextRole:
        return entry.weekdayText;
    case ConditionRole:
    case Qt::DisplayRole:
        return entry.condition;
    case IconRole:
        return entry.icon;
    case TemperatureRole:
        return entry.temperature;
    case HighTemperatureRole:
        return entry.highTemperature;
    case LowTemperatureRole:
        return entry.lowTemperature;
    case PrecipitationRole:
        return entry.precipitation;
    case HumidityRole:
        return entry.humidity;
    case WindSpeedRole:
        return entry.windSpeed;
    case WeatherCodeRole:
        return entry.weatherCode;
    default:
        return {};
    }
}

QHash<int, QByteArray> WeatherListModel::roleNames() const
{
    return {
        {TimeTextRole, "timeText"},
        {DateTextRole, "dateText"},
        {WeekdayTextRole, "weekdayText"},
        {ConditionRole, "condition"},
        {IconRole, "icon"},
        {TemperatureRole, "temperature"},
        {HighTemperatureRole, "highTemperature"},
        {LowTemperatureRole, "lowTemperature"},
        {PrecipitationRole, "precipitation"},
        {HumidityRole, "humidity"},
        {WindSpeedRole, "windSpeed"},
        {WeatherCodeRole, "weatherCode"}
    };
}

void WeatherListModel::setEntries(QVector<Entry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
}

void WeatherListModel::clear()
{
    if (m_entries.isEmpty())
        return;

    beginResetModel();
    m_entries.clear();
    endResetModel();
}
