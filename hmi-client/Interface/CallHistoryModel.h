#ifndef CALLHISTORYMODEL_H
#define CALLHISTORYMODEL_H

#include <QAbstractListModel>
#include <QVector>

struct CallHistoryEntry
{
    int id = -1;
    QString displayName;
    QString phone;
    QString direction;
    QString status;
    QString timestamp;
    QString relativeTime;
};

class CallHistoryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        HistoryIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        PhoneRole,
        DirectionRole,
        StatusRole,
        TimestampRole,
        RelativeTimeRole
    };
    Q_ENUM(Role)

    explicit CallHistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<CallHistoryEntry> &entries);

private:
    QVector<CallHistoryEntry> m_entries;
};

#endif
