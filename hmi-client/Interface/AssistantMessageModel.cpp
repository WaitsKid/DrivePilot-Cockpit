#include "AssistantMessageModel.h"

#include <QDateTime>

AssistantMessageModel::AssistantMessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AssistantMessageModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant AssistantMessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case TextRole:
    case Qt::DisplayRole:
        return entry.text;
    case SenderRole:
        return entry.sender;
    case TimeRole:
        return entry.time;
    case StatusRole:
        return entry.status;
    default:
        return {};
    }
}

QHash<int, QByteArray> AssistantMessageModel::roleNames() const
{
    return {
        {TextRole, "messageText"},
        {SenderRole, "sender"},
        {TimeRole, "timeText"},
        {StatusRole, "messageStatus"}
    };
}

void AssistantMessageModel::appendMessage(const QString &text,
                                          const QString &sender,
                                          const QString &status)
{
    const QString normalized = text.trimmed();
    if (normalized.isEmpty())
        return;

    const int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.append({normalized,
                      sender,
                      QDateTime::currentDateTime().toString(QStringLiteral("HH:mm")),
                      status});
    endInsertRows();
}

void AssistantMessageModel::clearMessages()
{
    if (m_entries.isEmpty())
        return;

    beginResetModel();
    m_entries.clear();
    endResetModel();
}
