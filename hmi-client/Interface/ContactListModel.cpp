#include "ContactListModel.h"

ContactListModel::ContactListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ContactListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visibleRows.size();
}

QVariant ContactListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleRows.size())
        return {};

    const ContactEntry &contact = m_contacts.at(m_visibleRows.at(index.row()));
    switch (role) {
    case ContactIdRole:
        return contact.id;
    case NameRole:
    case Qt::DisplayRole:
        return contact.name;
    case PhoneRole:
        return contact.phone;
    case InitialsRole:
        return initialsForName(contact.name);
    case AvatarColorRole:
        return contact.avatarColor;
    case FavoriteRole:
        return contact.favorite;
    default:
        return {};
    }
}

QHash<int, QByteArray> ContactListModel::roleNames() const
{
    return {
        {ContactIdRole, "contactId"},
        {NameRole, "name"},
        {PhoneRole, "phone"},
        {InitialsRole, "initials"},
        {AvatarColorRole, "avatarColor"},
        {FavoriteRole, "favorite"}
    };
}

void ContactListModel::setContacts(const QVector<ContactEntry> &contacts)
{
    beginResetModel();
    m_contacts = contacts;
    m_visibleRows.clear();

    for (int index = 0; index < m_contacts.size(); ++index) {
        const ContactEntry &contact = m_contacts.at(index);
        if (m_searchText.isEmpty()
            || contact.name.contains(m_searchText, Qt::CaseInsensitive)
            || contact.phone.contains(m_searchText, Qt::CaseInsensitive)) {
            m_visibleRows.append(index);
        }
    }

    endResetModel();
}

void ContactListModel::setSearchText(const QString &text)
{
    const QString normalized = text.trimmed();
    if (m_searchText == normalized)
        return;

    m_searchText = normalized;
    rebuildVisibleRows();
}

const ContactEntry *ContactListModel::contactById(int id) const
{
    for (const ContactEntry &contact : m_contacts) {
        if (contact.id == id)
            return &contact;
    }
    return nullptr;
}

int ContactListModel::totalCount() const
{
    return m_contacts.size();
}

void ContactListModel::rebuildVisibleRows()
{
    beginResetModel();
    m_visibleRows.clear();

    for (int index = 0; index < m_contacts.size(); ++index) {
        const ContactEntry &contact = m_contacts.at(index);
        if (m_searchText.isEmpty()
            || contact.name.contains(m_searchText, Qt::CaseInsensitive)
            || contact.phone.contains(m_searchText, Qt::CaseInsensitive)) {
            m_visibleRows.append(index);
        }
    }

    endResetModel();
}

QString ContactListModel::initialsForName(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return QStringLiteral("?");

    const QStringList parts = trimmed.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (parts.size() >= 2)
        return parts.first().left(1).toUpper() + parts.last().left(1).toUpper();

    return trimmed.left(2).toUpper();
}
