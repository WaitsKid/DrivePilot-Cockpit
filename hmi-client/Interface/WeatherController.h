#ifndef WEATHERCONTROLLER_H
#define WEATHERCONTROLLER_H

#include "CitySuggestionModel.h"
#include "WeatherListModel.h"

#include <QByteArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <qqmlintegration.h>

#ifdef BYD_HAS_QT_POSITIONING
class QGeoPositionInfoSource;
#endif

class WeatherController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Weather)

    Q_PROPERTY(QString locationQuery READ locationQuery WRITE setLocationQuery NOTIFY locationQueryChanged)
    Q_PROPERTY(QString locationName READ locationName NOTIFY locationNameChanged)
    Q_PROPERTY(double latitude READ latitude NOTIFY coordinatesChanged)
    Q_PROPERTY(double longitude READ longitude NOTIFY coordinatesChanged)

    Q_PROPERTY(int temperature READ temperature NOTIFY currentWeatherChanged)
    Q_PROPERTY(int apparentTemperature READ apparentTemperature NOTIFY currentWeatherChanged)
    Q_PROPERTY(int humidity READ humidity NOTIFY currentWeatherChanged)
    Q_PROPERTY(int windSpeed READ windSpeed NOTIFY currentWeatherChanged)
    Q_PROPERTY(int windDirection READ windDirection NOTIFY currentWeatherChanged)
    Q_PROPERTY(QString windDirectionName READ windDirectionName NOTIFY currentWeatherChanged)
    Q_PROPERTY(int visibility READ visibility NOTIFY currentWeatherChanged)
    Q_PROPERTY(int precipitationProbability READ precipitationProbability NOTIFY currentWeatherChanged)
    Q_PROPERTY(int weatherCode READ weatherCode NOTIFY currentWeatherChanged)
    Q_PROPERTY(QString condition READ condition NOTIFY currentWeatherChanged)
    Q_PROPERTY(QString conditionIcon READ conditionIcon NOTIFY currentWeatherChanged)

    Q_PROPERTY(int airQualityIndex READ airQualityIndex NOTIFY airQualityChanged)
    Q_PROPERTY(QString airQualityLevel READ airQualityLevel NOTIFY airQualityChanged)
    Q_PROPERTY(QString airQualityColor READ airQualityColor NOTIFY airQualityChanged)
    Q_PROPERTY(double pm25 READ pm25 NOTIFY airQualityChanged)
    Q_PROPERTY(double pm10 READ pm10 NOTIFY airQualityChanged)

    Q_PROPERTY(QString comfortAdvice READ comfortAdvice NOTIFY adviceChanged)
    Q_PROPERTY(QString drivingAdvice READ drivingAdvice NOTIFY adviceChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY statusChanged)
    Q_PROPERTY(QString dataSource READ dataSource NOTIFY statusChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY statusChanged)
    Q_PROPERTY(bool online READ online NOTIFY statusChanged)
    Q_PROPERTY(bool usingCache READ usingCache NOTIFY statusChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY statusChanged)

    Q_PROPERTY(bool locating READ locating NOTIFY locationStatusChanged)
    Q_PROPERTY(QString locationMethod READ locationMethod NOTIFY locationStatusChanged)
    Q_PROPERTY(bool suggestionsLoading READ suggestionsLoading NOTIFY suggestionStatusChanged)

    Q_PROPERTY(QAbstractItemModel *hourlyForecast READ hourlyForecast CONSTANT)
    Q_PROPERTY(QAbstractItemModel *dailyForecast READ dailyForecast CONSTANT)
    Q_PROPERTY(QAbstractItemModel *citySuggestions READ citySuggestions CONSTANT)

public:
    explicit WeatherController(QObject *parent = nullptr);

    QString locationQuery() const;
    void setLocationQuery(const QString &query);
    QString locationName() const;
    double latitude() const;
    double longitude() const;

    int temperature() const;
    int apparentTemperature() const;
    int humidity() const;
    int windSpeed() const;
    int windDirection() const;
    QString windDirectionName() const;
    int visibility() const;
    int precipitationProbability() const;
    int weatherCode() const;
    QString condition() const;
    QString conditionIcon() const;

    int airQualityIndex() const;
    QString airQualityLevel() const;
    QString airQualityColor() const;
    double pm25() const;
    double pm10() const;

    QString comfortAdvice() const;
    QString drivingAdvice() const;
    QString lastUpdated() const;
    QString dataSource() const;
    bool loading() const;
    bool online() const;
    bool usingCache() const;
    QString errorMessage() const;

    bool locating() const;
    QString locationMethod() const;
    bool suggestionsLoading() const;

    QAbstractItemModel *hourlyForecast();
    QAbstractItemModel *dailyForecast();
    QAbstractItemModel *citySuggestions();

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void searchLocation(const QString &query);
    Q_INVOKABLE void requestCitySuggestions(const QString &query);
    Q_INVOKABLE void selectCitySuggestion(int index);
    Q_INVOKABLE void showDefaultCitySuggestions();
    Q_INVOKABLE void clearCitySuggestions();
    Q_INVOKABLE void locateDevice();
    Q_INVOKABLE void useDemoData();

signals:
    void locationQueryChanged();
    void locationNameChanged();
    void coordinatesChanged();
    void currentWeatherChanged();
    void airQualityChanged();
    void adviceChanged();
    void statusChanged();
    void locationStatusChanged();
    void suggestionStatusChanged();
    void refreshFinished(bool success, const QString &message);

private:
    void requestForecast(quint64 serial);
    void requestAirQuality(quint64 serial);
    void finishRequest(bool success, const QString &message);

    void applyLocation(const QString &query,
                       const QString &displayName,
                       double latitude,
                       double longitude,
                       const QString &method,
                       bool remember);
    void requestClientLocation(double latitude, double longitude, bool systemPosition);
    void requestIpApproximateLocation();
    void finishLocationFailure(const QString &message);

    void populateDefaultSuggestions();
    void loadAmapConfiguration();
    QString locateConfigPath() const;
    bool amapConfigured() const;
    QVector<CitySuggestionModel::Entry> parseAmapDistrictPayload(
        const QByteArray &payload,
        const QString &query,
        QString *errorMessage) const;
    void rememberLocation(const CitySuggestionModel::Entry &entry);
    void loadRecentLocations();
    void saveRecentLocations() const;
    static QVector<CitySuggestionModel::Entry> popularCities();
    static CitySuggestionModel::Entry cityEntryFromJson(const QJsonObject &object,
                                                         const QString &source);
    static CitySuggestionModel::Entry districtEntryFromJson(
        const QJsonObject &object,
        const QStringList &parentNames,
        const QString &source);
    static void appendDistrictEntries(
        const QJsonObject &object,
        const QStringList &parentNames,
        QVector<CitySuggestionModel::Entry> *entries);
    static QString administrativeLevelText(const QString &level);
    static QString normalizedAdministrativeName(QString name);
    static int districtMatchScore(const QString &query, const QString &name);
    static double coordinateDistanceKilometers(double latitude1,
                                                double longitude1,
                                                double latitude2,
                                                double longitude2);
    static QString cityDetail(const QString &adminArea, const QString &country);

    bool parseForecast(const QJsonObject &root);
    void parseAirQuality(const QJsonObject &root);
    void updateAdvice();

    void loadPreferences();
    void savePreferences() const;
    bool loadCache();
    void saveCache() const;
    QString cacheFilePath() const;
    void populateDemoData();

    static QString weatherDescription(int code);
    static QString weatherIconForCode(int code);
    static QString airQualityDescription(int index);
    static QString airQualityColorForIndex(int index);
    static QString windDirectionLabel(int degrees);
    static QString buildLocationName(const QJsonObject &result);
    static int roundedJsonValue(const QJsonObject &object, const QString &key, int fallback = 0);

    QNetworkAccessManager m_networkManager;
    QTimer m_autoRefreshTimer;
    QTimer m_positionFallbackTimer;
    WeatherListModel m_hourlyModel;
    WeatherListModel m_dailyModel;
    CitySuggestionModel m_citySuggestionModel;

#ifdef BYD_HAS_QT_POSITIONING
    QGeoPositionInfoSource *m_positionSource = nullptr;
#endif

    QVector<CitySuggestionModel::Entry> m_recentLocations;
    quint64 m_requestSerial = 0;
    quint64 m_suggestionSerial = 0;
    quint64 m_locationSerial = 0;

    QString m_locationQuery = QStringLiteral("南京市雨花台区");
    QString m_locationName = QStringLiteral("南京市 雨花台区");
    double m_latitude = 31.995;
    double m_longitude = 118.780;

    int m_temperature = 26;
    int m_apparentTemperature = 27;
    int m_humidity = 63;
    int m_windSpeed = 12;
    int m_windDirection = 110;
    int m_visibility = 18;
    int m_precipitationProbability = 20;
    int m_weatherCode = 2;
    QString m_condition = QStringLiteral("多云");
    QString m_conditionIcon = QStringLiteral("🌤");

    int m_airQualityIndex = 42;
    QString m_airQualityLevel = QStringLiteral("优");
    QString m_airQualityColor = QStringLiteral("#54D6A6");
    double m_pm25 = 18.0;
    double m_pm10 = 31.0;

    QString m_comfortAdvice = QStringLiteral("体感舒适，适合通风和短途出行");
    QString m_drivingAdvice = QStringLiteral("道路状况良好，注意保持安全车距");
    QString m_lastUpdated;
    QString m_dataSource = QStringLiteral("演示数据");
    QString m_errorMessage;
    QString m_locationMethod = QStringLiteral("手动城市");
    bool m_loading = false;
    bool m_online = false;
    bool m_usingCache = false;
    bool m_locating = false;
    bool m_ipLocationPending = false;
    bool m_suggestionsLoading = false;

    QString m_amapWebServiceKey;

    QJsonObject m_lastForecastPayload;
    QJsonObject m_lastAirQualityPayload;
};

#endif
