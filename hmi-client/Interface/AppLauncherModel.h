#ifndef APPLAUNCHERMODEL_H
#define APPLAUNCHERMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QVector>
#include <qqmlintegration.h>

class AppLauncherModel : public QAbstractListModel
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(AppLauncher)

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(bool favoritesOnly READ favoritesOnly WRITE setFavoritesOnly NOTIFY favoritesOnlyChanged)
    Q_PROPERTY(bool frequentFirst READ frequentFirst WRITE setFrequentFirst NOTIFY frequentFirstChanged)
    Q_PROPERTY(QStringList categories READ categories CONSTANT)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultCountChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        IconRole,
        CategoryRole,
        FavoriteRole,
        TargetPageRole,
        AvailableRole,
        LaunchCountRole
    };
    Q_ENUM(Role)

    explicit AppLauncherModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString searchText() const;
    void setSearchText(const QString &text);

    QString category() const;
    void setCategory(const QString &category);

    bool favoritesOnly() const;
    void setFavoritesOnly(bool enabled);

    bool frequentFirst() const;
    void setFrequentFirst(bool enabled);

    QStringList categories() const;
    int resultCount() const;

    Q_INVOKABLE void toggleFavorite(int row);
    Q_INVOKABLE void markLaunched(int row);
    Q_INVOKABLE void clearFilters();

signals:
    void searchTextChanged();
    void categoryChanged();
    void favoritesOnlyChanged();
    void frequentFirstChanged();
    void resultCountChanged();

private:
    struct AppEntry {
        QString id;
        QString name;
        QString icon;
        QString category;
        int targetPage = -1;
        bool available = false;
        bool favorite = false;
        int launchCount = 0;
    };

    int sourceIndexForRow(int row) const;
    void rebuildVisibleRows();
    void loadState();
    void saveFavorites() const;
    void saveLaunchCounts() const;

    QVector<AppEntry> m_apps;
    QVector<int> m_visibleRows;
    QStringList m_categories;
    QString m_searchText;
    QString m_category;
    bool m_favoritesOnly = false;
    bool m_frequentFirst = false;
};

#endif
