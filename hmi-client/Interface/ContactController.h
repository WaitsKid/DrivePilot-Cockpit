#ifndef CONTACTCONTROLLER_H
#define CONTACTCONTROLLER_H

#include "CallHistoryModel.h"
#include "ContactListModel.h"

#include <QAbstractItemModel>
#include <QDateTime>
#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <qqmlintegration.h>

class ContactController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(PhoneBook)

    Q_PROPERTY(QAbstractItemModel *contacts READ contacts CONSTANT)
    Q_PROPERTY(QAbstractItemModel *callHistory READ callHistory CONSTANT)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString dialNumber READ dialNumber WRITE setDialNumber NOTIFY dialNumberChanged)
    Q_PROPERTY(int contactCount READ contactCount NOTIFY contactCountChanged)
    Q_PROPERTY(bool databaseReady READ databaseReady NOTIFY databaseReadyChanged)
    Q_PROPERTY(QString databasePath READ databasePath NOTIFY databaseReadyChanged)
    Q_PROPERTY(QString lastDialStatus READ lastDialStatus NOTIFY lastDialStatusChanged)
    Q_PROPERTY(QString lastDialedNumber READ lastDialedNumber NOTIFY lastDialStatusChanged)
    Q_PROPERTY(QString lastDialedName READ lastDialedName NOTIFY lastDialStatusChanged)

public:
    explicit ContactController(QObject *parent = nullptr);
    ~ContactController() override;

    QAbstractItemModel *contacts();
    QAbstractItemModel *callHistory();

    QString searchText() const;
    void setSearchText(const QString &text);

    QString dialNumber() const;
    void setDialNumber(const QString &number);

    int contactCount() const;
    bool databaseReady() const;
    QString databasePath() const;

    QString lastDialStatus() const;
    QString lastDialedNumber() const;
    QString lastDialedName() const;

    Q_INVOKABLE bool addContact(const QString &name, const QString &phone);
    Q_INVOKABLE bool updateContact(int contactId, const QString &name, const QString &phone);
    Q_INVOKABLE bool deleteContact(int contactId);
    Q_INVOKABLE void toggleFavorite(int contactId);

    Q_INVOKABLE bool dialContact(int contactId);
    Q_INVOKABLE bool dialHistoryNumber(const QString &name, const QString &phone);
    Q_INVOKABLE bool dialCurrentNumber();
    Q_INVOKABLE void appendDialCharacter(const QString &character);
    Q_INVOKABLE void removeLastDialCharacter();
    Q_INVOKABLE void clearDialNumber();
    Q_INVOKABLE void clearCallHistory();
    Q_INVOKABLE void refresh();

signals:
    void searchTextChanged();
    void dialNumberChanged();
    void contactCountChanged();
    void databaseReadyChanged();
    void lastDialStatusChanged();
    void noticeRequested(const QString &message);

private:
    bool initializeDatabase();
    bool createTables();
    void seedContacts();
    void importConfiguredContact();
    void reloadContacts();
    void reloadCallHistory();

    bool dialPhoneNumber(const QString &phone, const QString &displayName);
    bool insertCallHistory(const QString &phone,
                           const QString &displayName,
                           const QString &status);
    bool contactPhoneExists(const QString &phone, int exceptId = -1) const;

    QString normalizePhone(const QString &phone) const;
    QString chooseAvatarColor(const QString &name) const;
    QString relativeTimeText(const QDateTime &dateTime) const;
    QStringList configCandidates() const;

    ContactListModel m_contacts;
    CallHistoryModel m_callHistory;
    QSqlDatabase m_database;
    QString m_connectionName;
    QString m_databasePath;
    QString m_searchText;
    QString m_dialNumber;
    QString m_lastDialStatus;
    QString m_lastDialedNumber;
    QString m_lastDialedName;
    bool m_databaseReady = false;
};

#endif
