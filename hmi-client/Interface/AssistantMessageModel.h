#ifndef ASSISTANTMESSAGEMODEL_H
#define ASSISTANTMESSAGEMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class AssistantMessageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        TextRole = Qt::UserRole + 1,
        SenderRole,
        TimeRole,
        StatusRole
    };
    Q_ENUM(Role)

    struct Entry {
        QString text;
        QString sender;
        QString time;
        QString status;
    };

    explicit AssistantMessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void appendMessage(const QString &text,
                       const QString &sender,
                       const QString &status = QString());
    void clearMessages();

private:
    QVector<Entry> m_entries;
};

#endif
