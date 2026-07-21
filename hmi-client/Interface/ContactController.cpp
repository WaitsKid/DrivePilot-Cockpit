#include "ContactController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <QDebug>

namespace {
constexpr int kMaximumCallHistoryRows = 100;
}

ContactController::ContactController(QObject *parent)
    : QObject(parent)
    , m_contacts(nullptr)
    , m_callHistory(nullptr)
    , m_connectionName(QStringLiteral("phonebook_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
    m_databaseReady = initializeDatabase();
    emit databaseReadyChanged();

    if (m_databaseReady) {
        seedContacts();
        importConfiguredDemoContact();
        reloadContacts();
        reloadCallHistory();
    } else {
        emit noticeRequested(QStringLiteral("联系人数据库初始化失败，请查看控制台"));
    }
}

ContactController::~ContactController()
{
    if (m_database.isValid())
        m_database.close();

    m_database = {};
    QSqlDatabase::removeDatabase(m_connectionName);
}

QAbstractItemModel *ContactController::contacts()
{
    return &m_contacts;
}

QAbstractItemModel *ContactController::callHistory()
{
    return &m_callHistory;
}

QString ContactController::searchText() const
{
    return m_searchText;
}

void ContactController::setSearchText(const QString &text)
{
    const QString normalized = text.trimmed();
    if (m_searchText == normalized)
        return;

    m_searchText = normalized;
    m_contacts.setSearchText(m_searchText);
    emit searchTextChanged();
}

QString ContactController::dialNumber() const
{
    return m_dialNumber;
}

void ContactController::setDialNumber(const QString &number)
{
    const QString normalized = normalizePhone(number);
    if (m_dialNumber == normalized)
        return;

    m_dialNumber = normalized.left(24);
    emit dialNumberChanged();
}

int ContactController::contactCount() const
{
    return m_contacts.totalCount();
}

bool ContactController::databaseReady() const
{
    return m_databaseReady;
}

QString ContactController::databasePath() const
{
    return m_databasePath;
}

QString ContactController::lastDialStatus() const
{
    return m_lastDialStatus;
}

QString ContactController::lastDialedNumber() const
{
    return m_lastDialedNumber;
}

QString ContactController::lastDialedName() const
{
    return m_lastDialedName;
}

bool ContactController::addContact(const QString &name, const QString &phone)
{
    if (!m_databaseReady)
        return false;

    const QString normalizedName = name.trimmed();
    const QString normalizedPhone = normalizePhone(phone);
    if (normalizedName.isEmpty() || normalizedPhone.size() < 3) {
        emit noticeRequested(QStringLiteral("请输入有效的联系人姓名和号码"));
        return false;
    }

    if (contactPhoneExists(normalizedPhone)) {
        emit noticeRequested(QStringLiteral("该号码已经存在于联系人中"));
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO contacts(name, phone, avatar_color, favorite, created_at, updated_at) "
        "VALUES(?, ?, ?, 0, ?, ?)"));
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    query.addBindValue(normalizedName);
    query.addBindValue(normalizedPhone);
    query.addBindValue(chooseAvatarColor(normalizedName));
    query.addBindValue(now);
    query.addBindValue(now);

    if (!query.exec()) {
        qWarning() << "Insert contact failed:" << query.lastError();
        emit noticeRequested(QStringLiteral("联系人保存失败"));
        return false;
    }

    reloadContacts();
    emit contactCountChanged();
    emit noticeRequested(QStringLiteral("联系人已保存"));
    return true;
}

bool ContactController::updateContact(int contactId,
                                      const QString &name,
                                      const QString &phone)
{
    if (!m_databaseReady || contactId < 0)
        return false;

    const QString normalizedName = name.trimmed();
    const QString normalizedPhone = normalizePhone(phone);
    if (normalizedName.isEmpty() || normalizedPhone.size() < 3) {
        emit noticeRequested(QStringLiteral("请输入有效的联系人姓名和号码"));
        return false;
    }

    if (contactPhoneExists(normalizedPhone, contactId)) {
        emit noticeRequested(QStringLiteral("该号码已经存在于联系人中"));
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "UPDATE contacts SET name = ?, phone = ?, avatar_color = ?, updated_at = ? "
        "WHERE id = ?"));
    query.addBindValue(normalizedName);
    query.addBindValue(normalizedPhone);
    query.addBindValue(chooseAvatarColor(normalizedName));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
    query.addBindValue(contactId);

    if (!query.exec()) {
        qWarning() << "Update contact failed:" << query.lastError();
        emit noticeRequested(QStringLiteral("联系人更新失败"));
        return false;
    }

    reloadContacts();
    emit noticeRequested(QStringLiteral("联系人已更新"));
    return true;
}

bool ContactController::deleteContact(int contactId)
{
    if (!m_databaseReady || contactId < 0)
        return false;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM contacts WHERE id = ?"));
    query.addBindValue(contactId);
    if (!query.exec()) {
        qWarning() << "Delete contact failed:" << query.lastError();
        emit noticeRequested(QStringLiteral("联系人删除失败"));
        return false;
    }

    reloadContacts();
    emit contactCountChanged();
    emit noticeRequested(QStringLiteral("联系人已删除"));
    return true;
}

void ContactController::toggleFavorite(int contactId)
{
    if (!m_databaseReady || contactId < 0)
        return;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "UPDATE contacts SET favorite = CASE favorite WHEN 0 THEN 1 ELSE 0 END, "
        "updated_at = ? WHERE id = ?"));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
    query.addBindValue(contactId);
    if (!query.exec()) {
        qWarning() << "Toggle favorite failed:" << query.lastError();
        return;
    }

    reloadContacts();
}

bool ContactController::dialContact(int contactId)
{
    const ContactEntry *contact = m_contacts.contactById(contactId);
    if (!contact) {
        emit noticeRequested(QStringLiteral("联系人不存在或已被删除"));
        return false;
    }

    return dialPhoneNumber(contact->phone, contact->name);
}

bool ContactController::dialHistoryNumber(const QString &name, const QString &phone)
{
    return dialPhoneNumber(phone, name);
}

bool ContactController::dialCurrentNumber()
{
    return dialPhoneNumber(m_dialNumber, QStringLiteral("未知号码"));
}

void ContactController::appendDialCharacter(const QString &character)
{
    if (character.isEmpty())
        return;

    const QChar value = character.at(0);
    if (!value.isDigit() && value != QLatin1Char('*') && value != QLatin1Char('#')
        && !(value == QLatin1Char('+') && m_dialNumber.isEmpty())) {
        return;
    }

    if (m_dialNumber.size() >= 24)
        return;

    m_dialNumber.append(value);
    emit dialNumberChanged();
}

void ContactController::removeLastDialCharacter()
{
    if (m_dialNumber.isEmpty())
        return;

    m_dialNumber.chop(1);
    emit dialNumberChanged();
}

void ContactController::clearDialNumber()
{
    if (m_dialNumber.isEmpty())
        return;

    m_dialNumber.clear();
    emit dialNumberChanged();
}

void ContactController::clearCallHistory()
{
    if (!m_databaseReady)
        return;

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("DELETE FROM call_history"))) {
        qWarning() << "Clear call history failed:" << query.lastError();
        emit noticeRequested(QStringLiteral("通话记录清除失败"));
        return;
    }

    reloadCallHistory();
    emit noticeRequested(QStringLiteral("通话记录已清除"));
}

void ContactController::refresh()
{
    if (!m_databaseReady)
        return;

    importConfiguredDemoContact();
    reloadContacts();
    reloadCallHistory();
}

bool ContactController::initializeDatabase()
{
    const QString appDataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDirectory.isEmpty()) {
        qWarning() << "AppDataLocation is empty";
        return false;
    }

    QDir directory(appDataDirectory);
    if (!directory.mkpath(QStringLiteral("phonebook"))) {
        qWarning() << "Cannot create phonebook directory:" << appDataDirectory;
        return false;
    }

    m_databasePath = directory.filePath(QStringLiteral("phonebook/contacts.db"));
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(m_databasePath);

    if (!m_database.open()) {
        qWarning() << "Open contacts database failed:" << m_database.lastError();
        return false;
    }

    qInfo() << "Contacts database:" << m_databasePath;
    return createTables();
}

bool ContactController::createTables()
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS contacts("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "phone TEXT NOT NULL UNIQUE,"
            "avatar_color TEXT NOT NULL DEFAULT '#4C7BD9',"
            "favorite INTEGER NOT NULL DEFAULT 0,"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL)"))) {
        qWarning() << "Create contacts table failed:" << query.lastError();
        return false;
    }

    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS call_history("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "display_name TEXT NOT NULL,"
            "phone TEXT NOT NULL,"
            "direction TEXT NOT NULL,"
            "status TEXT NOT NULL,"
            "started_at TEXT NOT NULL,"
            "duration_seconds INTEGER NOT NULL DEFAULT 0)"))) {
        qWarning() << "Create call history table failed:" << query.lastError();
        return false;
    }

    query.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_contacts_name ON contacts(name)"));
    query.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_call_history_started_at "
        "ON call_history(started_at DESC)"));
    return true;
}

void ContactController::seedContacts()
{
    QSqlQuery countQuery(m_database);
    if (!countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM contacts"))
        || !countQuery.next() || countQuery.value(0).toInt() > 0) {
        return;
    }

    const QList<QPair<QString, QString>> defaults = {
        {QStringLiteral("车辆服务"), QStringLiteral("4000000000")},
        {QStringLiteral("演示联系人"), QStringLiteral("13800000000")}
    };

    for (const auto &entry : defaults) {
        QSqlQuery query(m_database);
        query.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO contacts(name, phone, avatar_color, favorite, created_at, updated_at) "
            "VALUES(?, ?, ?, 0, ?, ?)"));
        const QString now = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        query.addBindValue(entry.first);
        query.addBindValue(entry.second);
        query.addBindValue(chooseAvatarColor(entry.first));
        query.addBindValue(now);
        query.addBindValue(now);
        query.exec();
    }
}

void ContactController::importConfiguredDemoContact()
{
    for (const QString &path : configCandidates()) {
        QFile file(path);
        if (!file.exists() || !file.open(QIODevice::ReadOnly))
            continue;

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
            continue;

        const QJsonObject object = document.object();
        const QString phone = normalizePhone(object.value(QStringLiteral("demo_phone")).toString());
        if (phone.size() < 3)
            return;

        QString name = object.value(QStringLiteral("demo_contact_name")).toString().trimmed();
        if (name.isEmpty())
            name = QStringLiteral("备用机");

        QSqlQuery query(m_database);
        query.prepare(QStringLiteral(
            "INSERT INTO contacts(name, phone, avatar_color, favorite, created_at, updated_at) "
            "VALUES(?, ?, ?, 1, ?, ?) "
            "ON CONFLICT(phone) DO UPDATE SET name = excluded.name, "
            "avatar_color = excluded.avatar_color, favorite = 1, updated_at = excluded.updated_at"));
        const QString now = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        query.addBindValue(name);
        query.addBindValue(phone);
        query.addBindValue(chooseAvatarColor(name));
        query.addBindValue(now);
        query.addBindValue(now);
        if (!query.exec())
            qWarning() << "Import demo contact failed:" << query.lastError();
        return;
    }
}

void ContactController::reloadContacts()
{
    QVector<ContactEntry> contacts;
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, name, phone, avatar_color, favorite FROM contacts "
            "ORDER BY favorite DESC, name COLLATE NOCASE ASC"))) {
        qWarning() << "Load contacts failed:" << query.lastError();
        return;
    }

    while (query.next()) {
        ContactEntry contact;
        contact.id = query.value(0).toInt();
        contact.name = query.value(1).toString();
        contact.phone = query.value(2).toString();
        contact.avatarColor = query.value(3).toString();
        contact.favorite = query.value(4).toBool();
        contacts.append(contact);
    }

    m_contacts.setContacts(contacts);
}

void ContactController::reloadCallHistory()
{
    QVector<CallHistoryEntry> entries;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, display_name, phone, direction, status, started_at "
        "FROM call_history ORDER BY id DESC LIMIT ?"));
    query.addBindValue(kMaximumCallHistoryRows);
    if (!query.exec()) {
        qWarning() << "Load call history failed:" << query.lastError();
        return;
    }

    while (query.next()) {
        CallHistoryEntry entry;
        entry.id = query.value(0).toInt();
        entry.displayName = query.value(1).toString();
        entry.phone = query.value(2).toString();
        entry.direction = query.value(3).toString();
        entry.status = query.value(4).toString();
        const QDateTime dateTime = QDateTime::fromString(query.value(5).toString(), Qt::ISODateWithMs);
        entry.timestamp = dateTime.isValid()
                              ? dateTime.toString(QStringLiteral("M月d日 HH:mm"))
                              : query.value(5).toString();
        entry.relativeTime = relativeTimeText(dateTime);
        entries.append(entry);
    }

    m_callHistory.setEntries(entries);
}

bool ContactController::dialPhoneNumber(const QString &phone, const QString &displayName)
{
    const QString normalizedPhone = normalizePhone(phone);
    if (normalizedPhone.size() < 3) {
        emit noticeRequested(QStringLiteral("请输入有效的电话号码"));
        return false;
    }

    const QString normalizedName = displayName.trimmed().isEmpty()
                                       ? QStringLiteral("未知号码")
                                       : displayName.trimmed();
    const QUrl telephoneUrl(QStringLiteral("tel:%1").arg(normalizedPhone), QUrl::TolerantMode);
    const bool accepted = QDesktopServices::openUrl(telephoneUrl);

    m_lastDialedNumber = normalizedPhone;
    m_lastDialedName = normalizedName;
    m_lastDialStatus = accepted
                           ? QStringLiteral("已交给系统拨号服务")
                           : QStringLiteral("系统未配置 tel: 拨号处理程序");
    emit lastDialStatusChanged();

    insertCallHistory(normalizedPhone, normalizedName, m_lastDialStatus);
    reloadCallHistory();

    if (accepted) {
        emit noticeRequested(QStringLiteral("正在调用系统拨号服务：%1").arg(normalizedPhone));
    } else {
        emit noticeRequested(QStringLiteral(
            "无法直接拨号：请在 Windows 中配置手机连接或其他 tel: 处理程序"));
    }
    return accepted;
}

bool ContactController::insertCallHistory(const QString &phone,
                                          const QString &displayName,
                                          const QString &status)
{
    if (!m_databaseReady)
        return false;

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO call_history(display_name, phone, direction, status, started_at, duration_seconds) "
        "VALUES(?, ?, 'outgoing', ?, ?, 0)"));
    query.addBindValue(displayName);
    query.addBindValue(phone);
    query.addBindValue(status);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        qWarning() << "Insert call history failed:" << query.lastError();
        return false;
    }
    return true;
}

bool ContactController::contactPhoneExists(const QString &phone, int exceptId) const
{
    QSqlQuery query(m_database);
    if (exceptId >= 0) {
        query.prepare(QStringLiteral("SELECT 1 FROM contacts WHERE phone = ? AND id != ? LIMIT 1"));
        query.addBindValue(phone);
        query.addBindValue(exceptId);
    } else {
        query.prepare(QStringLiteral("SELECT 1 FROM contacts WHERE phone = ? LIMIT 1"));
        query.addBindValue(phone);
    }

    return query.exec() && query.next();
}

QString ContactController::normalizePhone(const QString &phone) const
{
    QString normalized;
    const QString trimmed = phone.trimmed();
    for (int index = 0; index < trimmed.size(); ++index) {
        const QChar character = trimmed.at(index);
        if (character.isDigit() || character == QLatin1Char('*') || character == QLatin1Char('#')) {
            normalized.append(character);
        } else if (character == QLatin1Char('+') && normalized.isEmpty()) {
            normalized.append(character);
        }
    }
    return normalized;
}

QString ContactController::chooseAvatarColor(const QString &name) const
{
    static const QStringList colors = {
        QStringLiteral("#4C7BD9"),
        QStringLiteral("#6277C7"),
        QStringLiteral("#3D8B8B"),
        QStringLiteral("#8B6BC5"),
        QStringLiteral("#C56B8A"),
        QStringLiteral("#C2864A"),
        QStringLiteral("#557D9C")
    };
    return colors.at(static_cast<quint64>(qHash(name)) % static_cast<quint64>(colors.size()));
}

QString ContactController::relativeTimeText(const QDateTime &dateTime) const
{
    if (!dateTime.isValid())
        return QString();

    const qint64 seconds = dateTime.secsTo(QDateTime::currentDateTime());
    if (seconds < 60)
        return QStringLiteral("刚刚");
    if (seconds < 3600)
        return QStringLiteral("%1 分钟前").arg(seconds / 60);
    if (seconds < 86400)
        return QStringLiteral("%1 小时前").arg(seconds / 3600);
    if (seconds < 172800)
        return QStringLiteral("昨天");
    return dateTime.toString(QStringLiteral("M月d日"));
}

QStringList ContactController::configCandidates() const
{
    QStringList candidates;
    const QString applicationDirectory = QCoreApplication::applicationDirPath();
    const QString workingDirectory = QDir::currentPath();

    auto appendUnique = [&candidates](const QString &path) {
        const QString cleanPath = QDir::cleanPath(path);
        if (!candidates.contains(cleanPath))
            candidates.append(cleanPath);
    };

    appendUnique(QDir(applicationDirectory).filePath(QStringLiteral("config.json")));
    appendUnique(QDir(workingDirectory).filePath(QStringLiteral("config.json")));
    appendUnique(QDir(applicationDirectory).filePath(QStringLiteral("../config.json")));
    appendUnique(QDir(applicationDirectory).filePath(QStringLiteral("../../config.json")));
    appendUnique(QDir(workingDirectory).filePath(QStringLiteral("../config.json")));
    return candidates;
}
