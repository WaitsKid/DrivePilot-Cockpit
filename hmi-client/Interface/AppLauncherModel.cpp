#include "AppLauncherModel.h"

#include "Interface.h"

#include <QSettings>
#include <QVariantMap>
#include <QtGlobal>
#include <algorithm>

namespace {
const QString kAllCategory = QStringLiteral("全部");
}

AppLauncherModel::AppLauncherModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_categories({kAllCategory,
                    QStringLiteral("车辆"),
                    QStringLiteral("出行"),
                    QStringLiteral("娱乐"),
                    QStringLiteral("工具"),
                    QStringLiteral("社交"),
                    QStringLiteral("学习"),
                    QStringLiteral("创作"),
                    QStringLiteral("效率")})
    , m_category(kAllCategory)
{
    m_apps = {
        {QStringLiteral("vehicle"), QStringLiteral("Vehicle"), QStringLiteral("qrc:/Images/Home/vehicle.png"), QStringLiteral("车辆"), Interface::PAGE_VEHICLE, true},
        {QStringLiteral("climate"), QStringLiteral("Climate"), QStringLiteral("qrc:/Images/ACBar/blow.png"), QStringLiteral("车辆"), Interface::PAGE_AC, true},
        {QStringLiteral("music"), QStringLiteral("Music"), QStringLiteral("qrc:/Images/App/Music.png"), QStringLiteral("娱乐"), Interface::PAGE_MUSIC, true},
        {QStringLiteral("settings"), QStringLiteral("Settings"), QStringLiteral("qrc:/Images/Settings/vehicle.png"), QStringLiteral("车辆"), Interface::PAGE_SETTINGS, true},
        {QStringLiteral("weather"), QStringLiteral("Weather"), QStringLiteral("qrc:/Images/Home/Weather/sun_clouds.png"), QStringLiteral("工具"), Interface::PAGE_WEATHER, true},
        {QStringLiteral("voice-assistant"), QStringLiteral("Voice Assistant"), QStringLiteral("qrc:/Images/App/Mimo.png"), QStringLiteral("工具"), Interface::PAGE_ASSISTANT, true},
        {QStringLiteral("maps"), QStringLiteral("Maps"), QStringLiteral("qrc:/Images/App/Maps.png"), QStringLiteral("出行"), Interface::PAGE_MAP, true},
        {QStringLiteral("calculator"), QStringLiteral("Calculator"), QStringLiteral("qrc:/Images/App/Caltulator.png"), QStringLiteral("工具"), Interface::PAGE_CALCULATOR, true},
        {QStringLiteral("video-center"), QStringLiteral("Video Center"), QStringLiteral("qrc:/Images/App/Podcast.png"), QStringLiteral("娱乐"), Interface::PAGE_VIDEO, true},
        {QStringLiteral("phone"), QStringLiteral("Phone"), QStringLiteral("qrc:/Images/ACBar/contact.png"), QStringLiteral("社交"), Interface::PAGE_CONTACTS, true},
        {QStringLiteral("duolingo"), QStringLiteral("Duolingo"), QStringLiteral("qrc:/Images/App/Duolinguo.png"), QStringLiteral("学习")},
        {QStringLiteral("picstart"), QStringLiteral("Picstart"), QStringLiteral("qrc:/Images/App/Picstart.png"), QStringLiteral("创作")},
        {QStringLiteral("dayone"), QStringLiteral("DayOne"), QStringLiteral("qrc:/Images/App/DayOne.png"), QStringLiteral("效率")},
        {QStringLiteral("vectornator"), QStringLiteral("Vector Studio"), QStringLiteral("qrc:/Images/App/Vectornator.png"), QStringLiteral("创作"), Interface::PAGE_VECTOR_STUDIO, true},
        {QStringLiteral("spark"), QStringLiteral("Spark"), QStringLiteral("qrc:/Images/App/Spark.png"), QStringLiteral("效率")}
    };

    loadState();
    rebuildVisibleRows();
}

int AppLauncherModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visibleRows.size();
}

QVariant AppLauncherModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleRows.size())
        return {};

    const AppEntry &app = m_apps.at(m_visibleRows.at(index.row()));
    switch (role) {
    case IdRole:
        return app.id;
    case NameRole:
    case Qt::DisplayRole:
        return app.name;
    case IconRole:
        return app.icon;
    case CategoryRole:
        return app.category;
    case FavoriteRole:
        return app.favorite;
    case TargetPageRole:
        return app.targetPage;
    case AvailableRole:
        return app.available;
    case LaunchCountRole:
        return app.launchCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> AppLauncherModel::roleNames() const
{
    return {
        {IdRole, "appId"},
        {NameRole, "name"},
        {IconRole, "icon"},
        {CategoryRole, "categoryName"},
        {FavoriteRole, "favorite"},
        {TargetPageRole, "targetPage"},
        {AvailableRole, "available"},
        {LaunchCountRole, "launchCount"}
    };
}

QString AppLauncherModel::searchText() const
{
    return m_searchText;
}

void AppLauncherModel::setSearchText(const QString &text)
{
    const QString normalized = text.trimmed();
    if (m_searchText == normalized)
        return;

    m_searchText = normalized;
    emit searchTextChanged();
    rebuildVisibleRows();
}

QString AppLauncherModel::category() const
{
    return m_category;
}

void AppLauncherModel::setCategory(const QString &category)
{
    const QString normalized = m_categories.contains(category) ? category : kAllCategory;
    if (m_category == normalized)
        return;

    m_category = normalized;
    emit categoryChanged();
    rebuildVisibleRows();
}

bool AppLauncherModel::favoritesOnly() const
{
    return m_favoritesOnly;
}

void AppLauncherModel::setFavoritesOnly(bool enabled)
{
    if (m_favoritesOnly == enabled)
        return;

    m_favoritesOnly = enabled;
    emit favoritesOnlyChanged();
    rebuildVisibleRows();
}

bool AppLauncherModel::frequentFirst() const
{
    return m_frequentFirst;
}

void AppLauncherModel::setFrequentFirst(bool enabled)
{
    if (m_frequentFirst == enabled)
        return;

    m_frequentFirst = enabled;
    emit frequentFirstChanged();
    rebuildVisibleRows();
}

QStringList AppLauncherModel::categories() const
{
    return m_categories;
}

int AppLauncherModel::resultCount() const
{
    return m_visibleRows.size();
}

void AppLauncherModel::toggleFavorite(int row)
{
    const int sourceIndex = sourceIndexForRow(row);
    if (sourceIndex < 0)
        return;

    m_apps[sourceIndex].favorite = !m_apps[sourceIndex].favorite;
    saveFavorites();

    if (m_favoritesOnly || m_frequentFirst) {
        rebuildVisibleRows();
        return;
    }

    const QModelIndex changedIndex = index(row, 0);
    emit dataChanged(changedIndex, changedIndex, {FavoriteRole});
}

void AppLauncherModel::markLaunched(int row)
{
    const int sourceIndex = sourceIndexForRow(row);
    if (sourceIndex < 0)
        return;

    ++m_apps[sourceIndex].launchCount;
    saveLaunchCounts();

    if (m_frequentFirst) {
        rebuildVisibleRows();
        return;
    }

    const QModelIndex changedIndex = index(row, 0);
    emit dataChanged(changedIndex, changedIndex, {LaunchCountRole});
}

void AppLauncherModel::clearFilters()
{
    bool resetRequired = false;

    if (!m_searchText.isEmpty()) {
        m_searchText.clear();
        emit searchTextChanged();
        resetRequired = true;
    }

    const QString all = kAllCategory;
    if (m_category != all) {
        m_category = all;
        emit categoryChanged();
        resetRequired = true;
    }

    if (m_favoritesOnly) {
        m_favoritesOnly = false;
        emit favoritesOnlyChanged();
        resetRequired = true;
    }

    if (m_frequentFirst) {
        m_frequentFirst = false;
        emit frequentFirstChanged();
        resetRequired = true;
    }

    if (resetRequired)
        rebuildVisibleRows();
}

int AppLauncherModel::sourceIndexForRow(int row) const
{
    if (row < 0 || row >= m_visibleRows.size())
        return -1;

    return m_visibleRows.at(row);
}

void AppLauncherModel::rebuildVisibleRows()
{
    beginResetModel();
    m_visibleRows.clear();

    const QString all = kAllCategory;
    for (int index = 0; index < m_apps.size(); ++index) {
        const AppEntry &app = m_apps.at(index);
        const bool categoryMatches = m_category == all || app.category == m_category;
        const bool favoriteMatches = !m_favoritesOnly || app.favorite;
        const bool searchMatches = m_searchText.isEmpty()
            || app.name.contains(m_searchText, Qt::CaseInsensitive)
            || app.category.contains(m_searchText, Qt::CaseInsensitive);

        if (categoryMatches && favoriteMatches && searchMatches)
            m_visibleRows.append(index);
    }

    if (m_frequentFirst) {
        std::stable_sort(m_visibleRows.begin(), m_visibleRows.end(), [this](int left, int right) {
            const AppEntry &leftApp = m_apps.at(left);
            const AppEntry &rightApp = m_apps.at(right);
            if (leftApp.launchCount != rightApp.launchCount)
                return leftApp.launchCount > rightApp.launchCount;
            if (leftApp.favorite != rightApp.favorite)
                return leftApp.favorite;
            return left < right;
        });
    }

    endResetModel();
    emit resultCountChanged();
}

void AppLauncherModel::loadState()
{
    QSettings settings;
    const QStringList favoriteIds = settings.value(QStringLiteral("appLauncher/favorites")).toStringList();
    QSet<QString> favorites;
    for (const QString &id : favoriteIds)
        favorites.insert(id);

    const QVariantMap launchCounts = settings.value(QStringLiteral("appLauncher/launchCounts")).toMap();

    for (AppEntry &app : m_apps) {
        app.favorite = favorites.contains(app.id);
        app.launchCount = qMax(0, launchCounts.value(app.id).toInt());
    }
}

void AppLauncherModel::saveFavorites() const
{
    QStringList favorites;
    for (const AppEntry &app : m_apps) {
        if (app.favorite)
            favorites.append(app.id);
    }

    QSettings settings;
    settings.setValue(QStringLiteral("appLauncher/favorites"), favorites);
}

void AppLauncherModel::saveLaunchCounts() const
{
    QVariantMap launchCounts;
    for (const AppEntry &app : m_apps)
        launchCounts.insert(app.id, app.launchCount);

    QSettings settings;
    settings.setValue(QStringLiteral("appLauncher/launchCounts"), launchCounts);
}
