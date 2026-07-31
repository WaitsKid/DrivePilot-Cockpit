#ifndef CONTACTLISTMODEL_H
#define CONTACTLISTMODEL_H

#include <QAbstractListModel>
#include <QVector>

struct ContactEntry
{
    int id = -1;
    QString name;
    QString phone;
    QString avatarColor;
    bool favorite = false;
};

class ContactListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        ContactIdRole = Qt::UserRole + 1,
        NameRole,
        PhoneRole,
        InitialsRole,
        AvatarColorRole,
        FavoriteRole
    };
    Q_ENUM(Role)

    explicit ContactListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setContacts(const QVector<ContactEntry> &contacts);
    void setSearchText(const QString &text);
    const ContactEntry *contactById(int id) const;
    int totalCount() const;

private:
    void rebuildVisibleRows();
    static QString initialsForName(const QString &name);

    QVector<ContactEntry> m_contacts;
    QVector<int> m_visibleRows;
    QString m_searchText;
};

#endif
