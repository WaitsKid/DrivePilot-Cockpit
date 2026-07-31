#include "CallHistoryModel.h"

CallHistoryModel::CallHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CallHistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant CallHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const CallHistoryEntry &entry = m_entries.at(index.row());
    switch (role) {
    case HistoryIdRole:
        return entry.id;
    case DisplayNameRole:
    case Qt::DisplayRole:
        return entry.displayName;
    case PhoneRole:
        return entry.phone;
    case DirectionRole:
        return entry.direction;
    case StatusRole:
        return entry.status;
    case TimestampRole:
        return entry.timestamp;
    case RelativeTimeRole:
        return entry.relativeTime;
    default:
        return {};
    }
}

QHash<int, QByteArray> CallHistoryModel::roleNames() const
{
    return {
        {HistoryIdRole, "historyId"},
        {DisplayNameRole, "displayName"},
        {PhoneRole, "phone"},
        {DirectionRole, "direction"},
        {StatusRole, "status"},
        {TimestampRole, "timestamp"},
        {RelativeTimeRole, "relativeTime"}
    };
}

void CallHistoryModel::setEntries(const QVector<CallHistoryEntry> &entries)
{
    beginResetModel();
    m_entries = entries;
    endResetModel();
}
