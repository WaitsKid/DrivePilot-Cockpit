#include "WeatherController.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QSet>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <QtGlobal>
#include <cmath>
#include <algorithm>
#include <utility>

#ifdef DRIVEPILOT_HAS_QT_POSITIONING
#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#endif

namespace {
constexpr int kNetworkTimeoutMilliseconds = 12000;
constexpr int kAutoRefreshMilliseconds = 30 * 60 * 1000;
constexpr int kHourlyItemCount = 24;
constexpr int kDailyItemCount = 7;
constexpr int kSuggestionCount = 20;
constexpr int kMaximumRecentLocations = 5;
constexpr int kPositionTimeoutMilliseconds = 8000;

int arrayInt(const QJsonArray &array, int index, int fallback = 0)
{
    if (index < 0 || index >= array.size() || array.at(index).isNull())
        return fallback;
    return qRound(array.at(index).toDouble(fallback));
}

QString arrayString(const QJsonArray &array, int index)
{
    if (index < 0 || index >= array.size())
        return {};
    return array.at(index).toString();
}

QString weekdayName(const QDate &date)
{
    static const QStringList names = {
        QString(),
        QStringLiteral("周一"),
        QStringLiteral("周二"),
        QStringLiteral("周三"),
        QStringLiteral("周四"),
        QStringLiteral("周五"),
        QStringLiteral("周六"),
        QStringLiteral("周日")
    };
    return names.value(date.dayOfWeek());
}

bool sameLocation(const CitySuggestionModel::Entry &left,
                  const CitySuggestionModel::Entry &right)
{
    if (!left.name.isEmpty() && left.name.compare(right.name, Qt::CaseInsensitive) == 0
        && left.adminArea.compare(right.adminArea, Qt::CaseInsensitive) == 0) {
        return true;
    }

    return qAbs(left.latitude - right.latitude) < 0.0001
        && qAbs(left.longitude - right.longitude) < 0.0001;
}
}

WeatherController::WeatherController(QObject *parent)
    : QObject(parent)
{
    loadPreferences();
    loadAmapConfiguration();
    loadRecentLocations();
    populateDefaultSuggestions();

    if (!loadCache())
        populateFallbackData();

    m_autoRefreshTimer.setInterval(kAutoRefreshMilliseconds);
    m_autoRefreshTimer.setTimerType(Qt::VeryCoarseTimer);
    connect(&m_autoRefreshTimer, &QTimer::timeout, this, [this]() {
        if (!m_loading && !m_locating)
            refresh();
    });
    m_autoRefreshTimer.start();

    m_positionFallbackTimer.setSingleShot(true);
    m_positionFallbackTimer.setInterval(kPositionTimeoutMilliseconds + 1200);
    connect(&m_positionFallbackTimer, &QTimer::timeout, this, [this]() {
        if (m_locating)
            requestIpApproximateLocation();
    });

#ifdef DRIVEPILOT_HAS_QT_POSITIONING
    m_positionSource = QGeoPositionInfoSource::createDefaultSource(this);
    if (m_positionSource) {
        m_positionSource->setPreferredPositioningMethods(
            QGeoPositionInfoSource::AllPositioningMethods);

        connect(m_positionSource,
                &QGeoPositionInfoSource::positionUpdated,
                this,
                [this](const QGeoPositionInfo &position) {
                    if (!m_locating || m_ipLocationPending)
                        return;

                    const QGeoCoordinate coordinate = position.coordinate();
                    if (!coordinate.isValid()) {
                        requestIpApproximateLocation();
                        return;
                    }

                    m_positionFallbackTimer.stop();
                    requestClientLocation(coordinate.latitude(),
                                          coordinate.longitude(),
                                          true);
                });

        connect(m_positionSource,
                &QGeoPositionInfoSource::errorOccurred,
                this,
                [this](QGeoPositionInfoSource::Error error) {
                    if (!m_locating || m_ipLocationPending
                        || error == QGeoPositionInfoSource::NoError)
                        return;
                    m_positionFallbackTimer.stop();
                    requestIpApproximateLocation();
                });
    }
#endif

    QTimer::singleShot(1000, this, &WeatherController::refresh);
}

QString WeatherController::locationQuery() const
{
    return m_locationQuery;
}

void WeatherController::setLocationQuery(const QString &query)
{
    const QString normalized = query.trimmed();
    if (normalized.isEmpty() || m_locationQuery == normalized)
        return;

    m_locationQuery = normalized;
    emit locationQueryChanged();
    savePreferences();
}

QString WeatherController::locationName() const
{
    return m_locationName;
}

double WeatherController::latitude() const
{
    return m_latitude;
}

double WeatherController::longitude() const
{
    return m_longitude;
}

int WeatherController::temperature() const
{
    return m_temperature;
}

int WeatherController::apparentTemperature() const
{
    return m_apparentTemperature;
}

int WeatherController::humidity() const
{
    return m_humidity;
}

int WeatherController::windSpeed() const
{
    return m_windSpeed;
}

int WeatherController::windDirection() const
{
    return m_windDirection;
}

QString WeatherController::windDirectionName() const
{
    return windDirectionLabel(m_windDirection);
}

int WeatherController::visibility() const
{
    return m_visibility;
}

int WeatherController::precipitationProbability() const
{
    return m_precipitationProbability;
}

int WeatherController::weatherCode() const
{
    return m_weatherCode;
}

QString WeatherController::condition() const
{
    return m_condition;
}

QString WeatherController::conditionIcon() const
{
    return m_conditionIcon;
}

int WeatherController::airQualityIndex() const
{
    return m_airQualityIndex;
}

QString WeatherController::airQualityLevel() const
{
    return m_airQualityLevel;
}

QString WeatherController::airQualityColor() const
{
    return m_airQualityColor;
}

double WeatherController::pm25() const
{
    return m_pm25;
}

double WeatherController::pm10() const
{
    return m_pm10;
}

QString WeatherController::comfortAdvice() const
{
    return m_comfortAdvice;
}

QString WeatherController::drivingAdvice() const
{
    return m_drivingAdvice;
}

QString WeatherController::lastUpdated() const
{
    return m_lastUpdated;
}

QString WeatherController::dataSource() const
{
    return m_dataSource;
}

bool WeatherController::loading() const
{
    return m_loading;
}

bool WeatherController::online() const
{
    return m_online;
}

bool WeatherController::usingCache() const
{
    return m_usingCache;
}

QString WeatherController::errorMessage() const
{
    return m_errorMessage;
}

bool WeatherController::locating() const
{
    return m_locating;
}

QString WeatherController::locationMethod() const
{
    return m_locationMethod;
}

bool WeatherController::suggestionsLoading() const
{
    return m_suggestionsLoading;
}

QAbstractItemModel *WeatherController::hourlyForecast()
{
    return &m_hourlyModel;
}

QAbstractItemModel *WeatherController::dailyForecast()
{
    return &m_dailyModel;
}

QAbstractItemModel *WeatherController::citySuggestions()
{
    return &m_citySuggestionModel;
}

void WeatherController::refresh()
{
    if (m_loading)
        return;

    ++m_requestSerial;
    m_loading = true;
    m_errorMessage.clear();
    emit statusChanged();
    requestForecast(m_requestSerial);
}

void WeatherController::searchLocation(const QString &query)
{
    const QString normalized = query.trimmed();
    if (normalized.isEmpty()) {
        emit refreshFinished(false, QStringLiteral("请输入中国省、市、区县或街道名称"));
        return;
    }

    loadAmapConfiguration();
    if (!amapConfigured()) {
        emit refreshFinished(false,
                             QStringLiteral("请先在 config.json 中填写 amap_web_service_key"));
        return;
    }

    ++m_requestSerial;
    const quint64 serial = m_requestSerial;
    m_loading = true;
    m_errorMessage.clear();
    emit statusChanged();

    QUrl url(QStringLiteral("https://restapi.amap.com/v3/config/district"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("key"), m_amapWebServiceKey);
    parameters.addQueryItem(QStringLiteral("keywords"), normalized);
    parameters.addQueryItem(QStringLiteral("subdistrict"), QStringLiteral("1"));
    parameters.addQueryItem(QStringLiteral("extensions"), QStringLiteral("base"));
    url.setQuery(parameters);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("QtInVehicleHMI/Weather/2.0"));
    request.setTransferTimeout(kNetworkTimeoutMilliseconds);

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, serial, normalized]() {
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString networkErrorText = reply->errorString();
        const QByteArray payload = reply->readAll();
        reply->deleteLater();

        if (serial != m_requestSerial)
            return;

        if (networkError != QNetworkReply::NoError) {
            finishRequest(false,
                          QStringLiteral("高德行政区搜索失败：%1").arg(networkErrorText));
            return;
        }

        QString parseMessage;
        QVector<CitySuggestionModel::Entry> entries =
            parseAmapDistrictPayload(payload, normalized, &parseMessage);
        if (entries.isEmpty()) {
            finishRequest(false,
                          parseMessage.isEmpty()
                              ? QStringLiteral("没有找到“%1”对应的中国行政区").arg(normalized)
                              : parseMessage);
            return;
        }

        const CitySuggestionModel::Entry entry = entries.constFirst();
        const QString displayName = entry.adminArea.isEmpty()
            ? entry.name
            : QStringLiteral("%1 %2").arg(entry.adminArea, entry.name);
        applyLocation(entry.name,
                      displayName,
                      entry.latitude,
                      entry.longitude,
                      QStringLiteral("高德行政区"),
                      true);
        rememberLocation(entry);
        requestForecast(serial);
    });
}

void WeatherController::requestCitySuggestions(const QString &query)
{
    const QString normalized = query.trimmed();
    ++m_suggestionSerial;
    const quint64 serial = m_suggestionSerial;

    if (normalized.isEmpty()) {
        m_suggestionsLoading = false;
        emit suggestionStatusChanged();
        populateDefaultSuggestions();
        return;
    }

    loadAmapConfiguration();
    if (!amapConfigured()) {
        m_suggestionsLoading = false;
        emit suggestionStatusChanged();
        m_citySuggestionModel.clear();
        return;
    }

    m_suggestionsLoading = true;
    emit suggestionStatusChanged();

    QUrl url(QStringLiteral("https://restapi.amap.com/v3/config/district"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("key"), m_amapWebServiceKey);
    parameters.addQueryItem(QStringLiteral("keywords"), normalized);
    parameters.addQueryItem(QStringLiteral("subdistrict"), QStringLiteral("1"));
    parameters.addQueryItem(QStringLiteral("extensions"), QStringLiteral("base"));
    url.setQuery(parameters);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("QtInVehicleHMI/Weather/2.0"));
    request.setTransferTimeout(kNetworkTimeoutMilliseconds);

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, serial, normalized]() {
        const QNetworkReply::NetworkError networkError = reply->error();
        const QByteArray payload = reply->readAll();
        reply->deleteLater();

        if (serial != m_suggestionSerial)
            return;

        m_suggestionsLoading = false;
        emit suggestionStatusChanged();

        if (networkError != QNetworkReply::NoError) {
            m_citySuggestionModel.clear();
            return;
        }

        QString parseMessage;
        QVector<CitySuggestionModel::Entry> entries =
            parseAmapDistrictPayload(payload, normalized, &parseMessage);
        m_citySuggestionModel.setEntries(std::move(entries));
    });
}

void WeatherController::selectCitySuggestion(int index)
{
    const CitySuggestionModel::Entry entry = m_citySuggestionModel.entryAt(index);
    if (entry.name.isEmpty()) {
        emit refreshFinished(false, QStringLiteral("城市候选项无效"));
        return;
    }

    ++m_requestSerial;
    const quint64 serial = m_requestSerial;
    m_loading = true;
    m_errorMessage.clear();
    emit statusChanged();

    const QString displayName = entry.adminArea.isEmpty()
        ? entry.name
        : QStringLiteral("%1 %2").arg(entry.adminArea, entry.name);
    applyLocation(entry.name,
                  displayName,
                  entry.latitude,
                  entry.longitude,
                  QStringLiteral("下拉选择"),
                  true);
    rememberLocation(entry);
    requestForecast(serial);
}

void WeatherController::showDefaultCitySuggestions()
{
    ++m_suggestionSerial;
    m_suggestionsLoading = false;
    emit suggestionStatusChanged();
    populateDefaultSuggestions();
}

void WeatherController::clearCitySuggestions()
{
    ++m_suggestionSerial;
    m_suggestionsLoading = false;
    emit suggestionStatusChanged();
    m_citySuggestionModel.clear();
}

void WeatherController::locateDevice()
{
    if (m_locating)
        return;

    ++m_locationSerial;
    m_locating = true;
    m_ipLocationPending = false;
    m_locationMethod = QStringLiteral("正在获取设备位置");
    emit locationStatusChanged();

#ifdef DRIVEPILOT_HAS_QT_POSITIONING
    if (m_positionSource) {
        m_positionFallbackTimer.start();
        m_positionSource->requestUpdate(kPositionTimeoutMilliseconds);
        return;
    }
#endif

    requestIpApproximateLocation();
}

void WeatherController::useFallbackData()
{
    ++m_requestSerial;
    populateFallbackData();
    m_loading = false;
    m_online = false;
    m_usingCache = false;
    m_dataSource = QStringLiteral("本地备用数据");
    m_errorMessage.clear();
    emit statusChanged();
    emit refreshFinished(true, QStringLiteral("已切换为本地备用天气"));
}

void WeatherController::requestForecast(quint64 serial)
{
    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("latitude"), QString::number(m_latitude, 'f', 6));
    parameters.addQueryItem(QStringLiteral("longitude"), QString::number(m_longitude, 'f', 6));
    parameters.addQueryItem(
        QStringLiteral("current"),
        QStringLiteral("temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m,wind_direction_10m,visibility"));
    parameters.addQueryItem(
        QStringLiteral("hourly"),
        QStringLiteral("temperature_2m,relative_humidity_2m,precipitation_probability,weather_code,wind_speed_10m"));
    parameters.addQueryItem(
        QStringLiteral("daily"),
        QStringLiteral("weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,wind_speed_10m_max,sunrise,sunset"));
    parameters.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    parameters.addQueryItem(QStringLiteral("forecast_days"), QString::number(kDailyItemCount));
    url.setQuery(parameters);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("QtInVehicleHMI/1.1"));
    request.setTransferTimeout(kNetworkTimeoutMilliseconds);

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, serial]() {
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString networkErrorText = reply->errorString();
        const QByteArray payload = reply->readAll();
        reply->deleteLater();

        if (serial != m_requestSerial)
            return;

        if (networkError != QNetworkReply::NoError) {
            finishRequest(false,
                          QStringLiteral("天气更新失败：%1").arg(networkErrorText));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishRequest(false, QStringLiteral("天气服务返回了无效数据"));
            return;
        }

        m_lastForecastPayload = document.object();
        if (!parseForecast(m_lastForecastPayload)) {
            finishRequest(false, QStringLiteral("天气数据字段不完整"));
            return;
        }

        requestAirQuality(serial);
    });
}

void WeatherController::requestAirQuality(quint64 serial)
{
    QUrl url(QStringLiteral("https://air-quality-api.open-meteo.com/v1/air-quality"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("latitude"), QString::number(m_latitude, 'f', 6));
    parameters.addQueryItem(QStringLiteral("longitude"), QString::number(m_longitude, 'f', 6));
    parameters.addQueryItem(QStringLiteral("current"),
                            QStringLiteral("us_aqi,pm10,pm2_5"));
    parameters.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    url.setQuery(parameters);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("QtInVehicleHMI/1.1"));
    request.setTransferTimeout(kNetworkTimeoutMilliseconds);

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, serial]() {
        const QNetworkReply::NetworkError networkError = reply->error();
        const QByteArray payload = reply->readAll();
        reply->deleteLater();

        if (serial != m_requestSerial)
            return;

        QString message = QStringLiteral("天气数据已更新");
        if (networkError == QNetworkReply::NoError) {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
            if (parseError.error == QJsonParseError::NoError && document.isObject()) {
                m_lastAirQualityPayload = document.object();
                parseAirQuality(m_lastAirQualityPayload);
            } else {
                message = QStringLiteral("天气已更新，空气质量数据暂不可用");
            }
        } else {
            message = QStringLiteral("天气已更新，空气质量请求失败");
        }

        m_lastUpdated = QDateTime::currentDateTime().toString(QStringLiteral("MM-dd HH:mm"));
        m_dataSource = QStringLiteral("Open-Meteo 在线数据");
        m_loading = false;
        m_online = true;
        m_usingCache = false;
        m_errorMessage.clear();
        updateAdvice();
        saveCache();
        emit statusChanged();
        emit refreshFinished(true, message);
    });
}

void WeatherController::finishRequest(bool success, const QString &message)
{
    m_loading = false;
    if (!success) {
        m_online = false;
        m_errorMessage = message;
        if (m_usingCache)
            m_dataSource = QStringLiteral("本地缓存（离线）");
        else if (m_dataSource.isEmpty())
            m_dataSource = QStringLiteral("本地备用数据");
    }
    emit statusChanged();
    emit refreshFinished(success, message);
}

void WeatherController::applyLocation(const QString &query,
                                      const QString &displayName,
                                      double latitude,
                                      double longitude,
                                      const QString &method,
                                      bool remember)
{
    const QString normalizedQuery = query.trimmed().isEmpty()
        ? displayName.trimmed()
        : query.trimmed();
    const QString normalizedName = displayName.trimmed().isEmpty()
        ? normalizedQuery
        : displayName.trimmed();

    const bool queryChanged = !normalizedQuery.isEmpty() && m_locationQuery != normalizedQuery;
    const bool nameChanged = !normalizedName.isEmpty() && m_locationName != normalizedName;
    const bool coordinateChanged = !qFuzzyCompare(m_latitude + 1.0, latitude + 1.0)
        || !qFuzzyCompare(m_longitude + 1.0, longitude + 1.0);
    const bool methodChanged = !method.isEmpty() && m_locationMethod != method;

    if (!normalizedQuery.isEmpty())
        m_locationQuery = normalizedQuery;
    if (!normalizedName.isEmpty())
        m_locationName = normalizedName;
    m_latitude = latitude;
    m_longitude = longitude;
    if (!method.isEmpty())
        m_locationMethod = method;

    if (queryChanged)
        emit locationQueryChanged();
    if (nameChanged)
        emit locationNameChanged();
    if (coordinateChanged)
        emit coordinatesChanged();
    if (methodChanged)
        emit locationStatusChanged();

    savePreferences();

    if (remember) {
        CitySuggestionModel::Entry entry;
        entry.name = m_locationQuery;
        entry.detail = m_locationName;
        entry.latitude = m_latitude;
        entry.longitude = m_longitude;
        entry.source = QStringLiteral("最近使用");
        rememberLocation(entry);
    }
}

void WeatherController::requestClientLocation(double latitude,
                                              double longitude,
                                              bool systemPosition)
{
    const quint64 serial = m_locationSerial;
    QUrl url(QStringLiteral("https://api.bigdatacloud.net/data/reverse-geocode-client"));
    QUrlQuery parameters;
    if (systemPosition) {
        parameters.addQueryItem(QStringLiteral("latitude"), QString::number(latitude, 'f', 7));
        parameters.addQueryItem(QStringLiteral("longitude"), QString::number(longitude, 'f', 7));
    }
    parameters.addQueryItem(QStringLiteral("localityLanguage"), QStringLiteral("zh"));
    url.setQuery(parameters);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("QtInVehicleHMI/1.1"));
    request.setTransferTimeout(kNetworkTimeoutMilliseconds);

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, serial, latitude, longitude, systemPosition]() {
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString errorText = reply->errorString();
        const QByteArray payload = reply->readAll();
        reply->deleteLater();

        if (serial != m_locationSerial || !m_locating)
            return;

        if (networkError != QNetworkReply::NoError) {
            if (systemPosition) {
                applyLocation(QStringLiteral("当前位置"),
                              QStringLiteral("当前位置"),
                              latitude,
                              longitude,
                              QStringLiteral("系统定位"),
                              true);
                m_locating = false;
                emit locationStatusChanged();

                ++m_requestSerial;
                m_loading = true;
                m_errorMessage.clear();
                emit statusChanged();
                requestForecast(m_requestSerial);
                return;
            }

            finishLocationFailure(QStringLiteral("设备定位失败：%1").arg(errorText));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishLocationFailure(QStringLiteral("定位服务返回了无效数据"));
            return;
        }

        const QJsonObject object = document.object();
        const double resolvedLatitude = systemPosition
            ? latitude : object.value(QStringLiteral("latitude")).toDouble(latitude);
        const double resolvedLongitude = systemPosition
            ? longitude : object.value(QStringLiteral("longitude")).toDouble(longitude);

        QString city = object.value(QStringLiteral("city")).toString().trimmed();
        const QString locality = object.value(QStringLiteral("locality")).toString().trimmed();
        const QString area = object.value(QStringLiteral("principalSubdivision")).toString().trimmed();
        const QString country = object.value(QStringLiteral("countryName")).toString().trimmed();
        if (city.isEmpty())
            city = locality;
        if (city.isEmpty())
            city = QStringLiteral("当前位置");

        QStringList displayParts;
        for (const QString &part : {area, city}) {
            if (!part.isEmpty() && !displayParts.contains(part))
                displayParts.append(part);
        }
        if (displayParts.isEmpty() && !country.isEmpty())
            displayParts.append(country);

        const QString method = systemPosition
            ? QStringLiteral("系统定位")
            : QStringLiteral("IP 近似定位");
        applyLocation(city,
                      displayParts.isEmpty() ? city : displayParts.join(QLatin1Char(' ')),
                      resolvedLatitude,
                      resolvedLongitude,
                      method,
                      true);

        m_locating = false;
        m_ipLocationPending = false;
        emit locationStatusChanged();

        ++m_requestSerial;
        m_loading = true;
        m_errorMessage.clear();
        emit statusChanged();
        requestForecast(m_requestSerial);
    });
}

void WeatherController::requestIpApproximateLocation()
{
    if (!m_locating || m_ipLocationPending)
        return;

    m_ipLocationPending = true;
    m_positionFallbackTimer.stop();
    m_locationMethod = QStringLiteral("正在使用 IP 近似定位");
    emit locationStatusChanged();
    requestClientLocation(0.0, 0.0, false);
}

void WeatherController::finishLocationFailure(const QString &message)
{
    m_positionFallbackTimer.stop();
    m_locating = false;
    m_ipLocationPending = false;
    m_locationMethod = QStringLiteral("定位失败");
    emit locationStatusChanged();
    emit refreshFinished(false, message);
}

void WeatherController::loadAmapConfiguration()
{
    m_amapWebServiceKey.clear();

    QFile file(locateConfigPath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return;

    const QString value = document.object()
                              .value(QStringLiteral("amap_web_service_key"))
                              .toString()
                              .trimmed();
    if (value.isEmpty() || value.contains(QStringLiteral("你的高德")))
        return;

    m_amapWebServiceKey = value;
}

QString WeatherController::locateConfigPath() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString currentDir = QDir::currentPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/config.json"),
        currentDir + QStringLiteral("/config.json"),
        QDir(appDir).absoluteFilePath(QStringLiteral("../config.json")),
        QDir(currentDir).absoluteFilePath(QStringLiteral("../config.json"))
    };

    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate))
            return QFileInfo(candidate).absoluteFilePath();
    }
    return candidates.constFirst();
}

bool WeatherController::amapConfigured() const
{
    return !m_amapWebServiceKey.trimmed().isEmpty();
}

QVector<CitySuggestionModel::Entry> WeatherController::parseAmapDistrictPayload(
    const QByteArray &payload,
    const QString &query,
    QString *errorMessage) const
{
    if (errorMessage)
        errorMessage->clear();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("高德行政区服务返回了无效 JSON");
        return {};
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("status")).toString() != QStringLiteral("1")) {
        if (errorMessage) {
            const QString info = root.value(QStringLiteral("info")).toString();
            *errorMessage = info.isEmpty()
                ? QStringLiteral("高德行政区服务请求失败")
                : QStringLiteral("高德行政区服务：%1").arg(info);
        }
        return {};
    }

    QVector<CitySuggestionModel::Entry> entries;
    const QJsonArray districts = root.value(QStringLiteral("districts")).toArray();
    for (const QJsonValue &value : districts) {
        if (value.isObject())
            appendDistrictEntries(value.toObject(), {}, &entries);
    }

    QSet<QString> seen;
    QVector<CitySuggestionModel::Entry> uniqueEntries;
    uniqueEntries.reserve(entries.size());
    for (const CitySuggestionModel::Entry &entry : std::as_const(entries)) {
        if (entry.name.isEmpty()
            || (qFuzzyIsNull(entry.latitude) && qFuzzyIsNull(entry.longitude))
            || qAbs(entry.latitude) > 90.0
            || qAbs(entry.longitude) > 180.0) {
            continue;
        }

        const QString uniqueKey = QStringLiteral("%1|%2|%3|%4|%5")
                                      .arg(entry.name,
                                           entry.adcode,
                                           entry.level,
                                           QString::number(entry.latitude, 'f', 5),
                                           QString::number(entry.longitude, 'f', 5));
        if (seen.contains(uniqueKey))
            continue;
        seen.insert(uniqueKey);
        uniqueEntries.append(entry);
    }

    auto levelPriority = [](const QString &level) {
        if (level == QStringLiteral("city"))
            return 4;
        if (level == QStringLiteral("district"))
            return 3;
        if (level == QStringLiteral("province"))
            return 2;
        if (level == QStringLiteral("street"))
            return 1;
        return 0;
    };

    std::stable_sort(uniqueEntries.begin(),
                     uniqueEntries.end(),
                     [this, &query, &levelPriority](const CitySuggestionModel::Entry &left,
                                                   const CitySuggestionModel::Entry &right) {
        const int leftScore = districtMatchScore(query, left.name);
        const int rightScore = districtMatchScore(query, right.name);
        if (leftScore != rightScore)
            return leftScore > rightScore;

        const int leftLevel = levelPriority(left.level);
        const int rightLevel = levelPriority(right.level);
        if (leftLevel != rightLevel)
            return leftLevel > rightLevel;

        const double leftDistance = coordinateDistanceKilometers(
            m_latitude, m_longitude, left.latitude, left.longitude);
        const double rightDistance = coordinateDistanceKilometers(
            m_latitude, m_longitude, right.latitude, right.longitude);
        if (!qFuzzyCompare(leftDistance + 1.0, rightDistance + 1.0))
            return leftDistance < rightDistance;

        return left.name.localeAwareCompare(right.name) < 0;
    });

    if (uniqueEntries.size() > kSuggestionCount)
        uniqueEntries.resize(kSuggestionCount);
    return uniqueEntries;
}

CitySuggestionModel::Entry WeatherController::districtEntryFromJson(
    const QJsonObject &object,
    const QStringList &parentNames,
    const QString &source)
{
    CitySuggestionModel::Entry entry;
    entry.name = object.value(QStringLiteral("name")).toString().trimmed();
    entry.adcode = object.value(QStringLiteral("adcode")).toString().trimmed();
    entry.level = object.value(QStringLiteral("level")).toString().trimmed();
    entry.country = QStringLiteral("中国");
    entry.adminArea = parentNames.join(QStringLiteral(" · "));
    entry.source = source;

    const QString center = object.value(QStringLiteral("center")).toString();
    const QStringList coordinate = center.split(QLatin1Char(','));
    if (coordinate.size() == 2) {
        bool longitudeOk = false;
        bool latitudeOk = false;
        entry.longitude = coordinate.at(0).toDouble(&longitudeOk);
        entry.latitude = coordinate.at(1).toDouble(&latitudeOk);
        if (!longitudeOk || !latitudeOk) {
            entry.longitude = 0.0;
            entry.latitude = 0.0;
        }
    }

    QStringList detailParts = parentNames;
    const QString levelText = administrativeLevelText(entry.level);
    if (!levelText.isEmpty())
        detailParts.append(levelText);
    entry.detail = detailParts.join(QStringLiteral(" · "));
    return entry;
}

void WeatherController::appendDistrictEntries(
    const QJsonObject &object,
    const QStringList &parentNames,
    QVector<CitySuggestionModel::Entry> *entries)
{
    if (!entries || entries->size() >= 120)
        return;

    const QString level = object.value(QStringLiteral("level")).toString();
    const QString name = object.value(QStringLiteral("name")).toString().trimmed();

    QStringList childParents = parentNames;
    if (level != QStringLiteral("country") && !name.isEmpty()) {
        const CitySuggestionModel::Entry entry =
            districtEntryFromJson(object, parentNames, QStringLiteral("高德行政区"));
        if (!entry.name.isEmpty())
            entries->append(entry);
        childParents.append(name);
    }

    const QJsonArray children = object.value(QStringLiteral("districts")).toArray();
    for (const QJsonValue &child : children) {
        if (entries->size() >= 120)
            break;
        if (child.isObject())
            appendDistrictEntries(child.toObject(), childParents, entries);
    }
}

QString WeatherController::administrativeLevelText(const QString &level)
{
    if (level == QStringLiteral("province"))
        return QStringLiteral("省级行政区");
    if (level == QStringLiteral("city"))
        return QStringLiteral("市级行政区");
    if (level == QStringLiteral("district"))
        return QStringLiteral("区县");
    if (level == QStringLiteral("street"))
        return QStringLiteral("街道/乡镇");
    return {};
}

QString WeatherController::normalizedAdministrativeName(QString name)
{
    name = name.trimmed().toLower();
    static const QStringList suffixes = {
        QStringLiteral("特别行政区"),
        QStringLiteral("维吾尔自治区"),
        QStringLiteral("壮族自治区"),
        QStringLiteral("回族自治区"),
        QStringLiteral("自治区"),
        QStringLiteral("自治州"),
        QStringLiteral("地区"),
        QStringLiteral("街道"),
        QStringLiteral("省"),
        QStringLiteral("市"),
        QStringLiteral("区"),
        QStringLiteral("县"),
        QStringLiteral("旗"),
        QStringLiteral("镇"),
        QStringLiteral("乡")
    };

    bool removed = true;
    while (removed) {
        removed = false;
        for (const QString &suffix : suffixes) {
            if (name.endsWith(suffix) && name.size() > suffix.size()) {
                name.chop(suffix.size());
                removed = true;
                break;
            }
        }
    }
    return name;
}

int WeatherController::districtMatchScore(const QString &query, const QString &name)
{
    const QString rawQuery = query.trimmed().toLower();
    const QString rawName = name.trimmed().toLower();
    const QString normalizedQuery = normalizedAdministrativeName(rawQuery);
    const QString normalizedName = normalizedAdministrativeName(rawName);

    if (rawQuery == rawName)
        return 10000;
    if (!normalizedQuery.isEmpty() && normalizedQuery == normalizedName)
        return 9600;
    if (rawName.startsWith(rawQuery) || normalizedName.startsWith(normalizedQuery))
        return 8200;
    if (rawName.contains(rawQuery) || normalizedName.contains(normalizedQuery))
        return 7000;

    int commonCharacters = 0;
    for (const QChar character : normalizedQuery) {
        if (normalizedName.contains(character))
            ++commonCharacters;
    }
    return commonCharacters * 120 - qAbs(normalizedName.size() - normalizedQuery.size()) * 5;
}

double WeatherController::coordinateDistanceKilometers(double latitude1,
                                                        double longitude1,
                                                        double latitude2,
                                                        double longitude2)
{
    constexpr double earthRadiusKilometers = 6371.0088;
    constexpr double degreesToRadians = 3.14159265358979323846 / 180.0;
    const double lat1 = latitude1 * degreesToRadians;
    const double lat2 = latitude2 * degreesToRadians;
    const double deltaLatitude = (latitude2 - latitude1) * degreesToRadians;
    const double deltaLongitude = (longitude2 - longitude1) * degreesToRadians;
    const double sinLatitude = std::sin(deltaLatitude / 2.0);
    const double sinLongitude = std::sin(deltaLongitude / 2.0);
    const double a = sinLatitude * sinLatitude
        + std::cos(lat1) * std::cos(lat2) * sinLongitude * sinLongitude;
    return earthRadiusKilometers * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

void WeatherController::populateDefaultSuggestions()
{
    QVector<CitySuggestionModel::Entry> entries;
    entries.reserve(m_recentLocations.size() + popularCities().size());

    for (CitySuggestionModel::Entry entry : std::as_const(m_recentLocations)) {
        entry.source = QStringLiteral("最近使用");
        entries.append(entry);
    }

    const QVector<CitySuggestionModel::Entry> popular = popularCities();
    for (CitySuggestionModel::Entry entry : popular) {
        bool duplicate = false;
        for (const CitySuggestionModel::Entry &existing : std::as_const(entries)) {
            if (sameLocation(existing, entry)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
            entries.append(entry);
    }

    m_citySuggestionModel.setEntries(std::move(entries));
}

void WeatherController::rememberLocation(const CitySuggestionModel::Entry &entry)
{
    if (entry.name.trimmed().isEmpty())
        return;

    CitySuggestionModel::Entry recent = entry;
    recent.source = QStringLiteral("最近使用");
    if (recent.detail.isEmpty())
        recent.detail = cityDetail(recent.adminArea, recent.country);

    for (int index = m_recentLocations.size() - 1; index >= 0; --index) {
        if (sameLocation(m_recentLocations.at(index), recent))
            m_recentLocations.removeAt(index);
    }

    m_recentLocations.prepend(recent);
    while (m_recentLocations.size() > kMaximumRecentLocations)
        m_recentLocations.removeLast();

    saveRecentLocations();
}

void WeatherController::loadRecentLocations()
{
    QSettings settings;
    const QByteArray json = settings.value(QStringLiteral("weather/recentLocations")).toByteArray();
    if (json.isEmpty())
        return;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray())
        return;

    const QJsonArray array = document.array();
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        CitySuggestionModel::Entry entry;
        entry.name = object.value(QStringLiteral("name")).toString();
        entry.detail = object.value(QStringLiteral("detail")).toString();
        entry.country = object.value(QStringLiteral("country")).toString();
        entry.adminArea = object.value(QStringLiteral("adminArea")).toString();
        entry.latitude = object.value(QStringLiteral("latitude")).toDouble();
        entry.longitude = object.value(QStringLiteral("longitude")).toDouble();
        entry.source = QStringLiteral("最近使用");
        entry.adcode = object.value(QStringLiteral("adcode")).toString();
        entry.level = object.value(QStringLiteral("level")).toString();
        if (!entry.name.isEmpty())
            m_recentLocations.append(entry);
        if (m_recentLocations.size() >= kMaximumRecentLocations)
            break;
    }
}

void WeatherController::saveRecentLocations() const
{
    QJsonArray array;
    for (const CitySuggestionModel::Entry &entry : m_recentLocations) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), entry.name);
        object.insert(QStringLiteral("detail"), entry.detail);
        object.insert(QStringLiteral("country"), entry.country);
        object.insert(QStringLiteral("adminArea"), entry.adminArea);
        object.insert(QStringLiteral("latitude"), entry.latitude);
        object.insert(QStringLiteral("longitude"), entry.longitude);
        object.insert(QStringLiteral("adcode"), entry.adcode);
        object.insert(QStringLiteral("level"), entry.level);
        array.append(object);
    }

    QSettings settings;
    settings.setValue(QStringLiteral("weather/recentLocations"),
                      QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QVector<CitySuggestionModel::Entry> WeatherController::popularCities()
{
    return {
        {QStringLiteral("杭州"), QStringLiteral("浙江 · 中国"), QStringLiteral("中国"), QStringLiteral("浙江"), 30.2741, 120.1551, QStringLiteral("热门城市")},
        {QStringLiteral("南京"), QStringLiteral("江苏 · 中国"), QStringLiteral("中国"), QStringLiteral("江苏"), 32.0603, 118.7969, QStringLiteral("热门城市")},
        {QStringLiteral("上海"), QStringLiteral("上海 · 中国"), QStringLiteral("中国"), QStringLiteral("上海"), 31.2304, 121.4737, QStringLiteral("热门城市")},
        {QStringLiteral("北京"), QStringLiteral("北京 · 中国"), QStringLiteral("中国"), QStringLiteral("北京"), 39.9042, 116.4074, QStringLiteral("热门城市")},
        {QStringLiteral("广州"), QStringLiteral("广东 · 中国"), QStringLiteral("中国"), QStringLiteral("广东"), 23.1291, 113.2644, QStringLiteral("热门城市")},
        {QStringLiteral("深圳"), QStringLiteral("广东 · 中国"), QStringLiteral("中国"), QStringLiteral("广东"), 22.5431, 114.0579, QStringLiteral("热门城市")},
        {QStringLiteral("成都"), QStringLiteral("四川 · 中国"), QStringLiteral("中国"), QStringLiteral("四川"), 30.5728, 104.0668, QStringLiteral("热门城市")},
        {QStringLiteral("西安"), QStringLiteral("陕西 · 中国"), QStringLiteral("中国"), QStringLiteral("陕西"), 34.3416, 108.9398, QStringLiteral("热门城市")}
    };
}

CitySuggestionModel::Entry WeatherController::cityEntryFromJson(const QJsonObject &object,
                                                                 const QString &source)
{
    CitySuggestionModel::Entry entry;
    entry.name = object.value(QStringLiteral("name")).toString().trimmed();
    entry.country = object.value(QStringLiteral("country")).toString().trimmed();
    entry.adminArea = object.value(QStringLiteral("admin1")).toString().trimmed();
    entry.detail = cityDetail(entry.adminArea, entry.country);
    entry.latitude = object.value(QStringLiteral("latitude")).toDouble();
    entry.longitude = object.value(QStringLiteral("longitude")).toDouble();
    entry.source = source;
    return entry;
}

QString WeatherController::cityDetail(const QString &adminArea, const QString &country)
{
    QStringList parts;
    for (const QString &part : {adminArea.trimmed(), country.trimmed()}) {
        if (!part.isEmpty() && !parts.contains(part))
            parts.append(part);
    }
    return parts.join(QStringLiteral(" · "));
}

bool WeatherController::parseForecast(const QJsonObject &root)
{
    const QJsonObject current = root.value(QStringLiteral("current")).toObject();
    const QJsonObject hourly = root.value(QStringLiteral("hourly")).toObject();
    const QJsonObject daily = root.value(QStringLiteral("daily")).toObject();
    if (current.isEmpty() || hourly.isEmpty() || daily.isEmpty())
        return false;

    m_temperature = roundedJsonValue(current, QStringLiteral("temperature_2m"), m_temperature);
    m_apparentTemperature = roundedJsonValue(current,
                                              QStringLiteral("apparent_temperature"),
                                              m_apparentTemperature);
    m_humidity = roundedJsonValue(current,
                                  QStringLiteral("relative_humidity_2m"),
                                  m_humidity);
    m_windSpeed = roundedJsonValue(current, QStringLiteral("wind_speed_10m"), m_windSpeed);
    m_windDirection = roundedJsonValue(current,
                                       QStringLiteral("wind_direction_10m"),
                                       m_windDirection);
    m_visibility = qMax(0,
                        qRound(current.value(QStringLiteral("visibility")).toDouble(
                                   m_visibility * 1000.0)
                               / 1000.0));
    m_weatherCode = roundedJsonValue(current, QStringLiteral("weather_code"), m_weatherCode);
    m_condition = weatherDescription(m_weatherCode);
    m_conditionIcon = weatherIconForCode(m_weatherCode);

    const QJsonArray hourlyTimes = hourly.value(QStringLiteral("time")).toArray();
    const QJsonArray hourlyTemperatures = hourly.value(QStringLiteral("temperature_2m")).toArray();
    const QJsonArray hourlyHumidity = hourly.value(QStringLiteral("relative_humidity_2m")).toArray();
    const QJsonArray hourlyPrecipitation = hourly.value(QStringLiteral("precipitation_probability")).toArray();
    const QJsonArray hourlyCodes = hourly.value(QStringLiteral("weather_code")).toArray();
    const QJsonArray hourlyWind = hourly.value(QStringLiteral("wind_speed_10m")).toArray();

    const QString currentTime = current.value(QStringLiteral("time")).toString();
    int startIndex = 0;
    for (int index = 0; index < hourlyTimes.size(); ++index) {
        if (arrayString(hourlyTimes, index) >= currentTime) {
            startIndex = index;
            break;
        }
    }

    m_precipitationProbability = arrayInt(hourlyPrecipitation, startIndex, 0);

    QVector<WeatherListModel::Entry> hourlyEntries;
    hourlyEntries.reserve(kHourlyItemCount);
    for (int offset = 0; offset < kHourlyItemCount; ++offset) {
        const int index = startIndex + offset;
        if (index >= hourlyTimes.size())
            break;

        const QDateTime dateTime = QDateTime::fromString(arrayString(hourlyTimes, index),
                                                         Qt::ISODate);
        const int code = arrayInt(hourlyCodes, index, 0);
        WeatherListModel::Entry entry;
        entry.timeText = offset == 0
            ? QStringLiteral("现在")
            : dateTime.time().toString(QStringLiteral("HH:mm"));
        entry.dateText = dateTime.date().toString(QStringLiteral("M/d"));
        entry.condition = weatherDescription(code);
        entry.icon = weatherIconForCode(code);
        entry.temperature = arrayInt(hourlyTemperatures, index, m_temperature);
        entry.precipitation = arrayInt(hourlyPrecipitation, index, 0);
        entry.humidity = arrayInt(hourlyHumidity, index, m_humidity);
        entry.windSpeed = arrayInt(hourlyWind, index, m_windSpeed);
        entry.weatherCode = code;
        hourlyEntries.append(entry);
    }
    m_hourlyModel.setEntries(std::move(hourlyEntries));

    const QJsonArray dailyTimes = daily.value(QStringLiteral("time")).toArray();
    const QJsonArray dailyCodes = daily.value(QStringLiteral("weather_code")).toArray();
    const QJsonArray dailyHighs = daily.value(QStringLiteral("temperature_2m_max")).toArray();
    const QJsonArray dailyLows = daily.value(QStringLiteral("temperature_2m_min")).toArray();
    const QJsonArray dailyPrecipitation = daily.value(QStringLiteral("precipitation_probability_max")).toArray();
    const QJsonArray dailyWind = daily.value(QStringLiteral("wind_speed_10m_max")).toArray();

    QVector<WeatherListModel::Entry> dailyEntries;
    const int dailyCount = qMin(kDailyItemCount, dailyTimes.size());
    dailyEntries.reserve(dailyCount);
    for (int index = 0; index < dailyCount; ++index) {
        const QDate date = QDate::fromString(arrayString(dailyTimes, index), Qt::ISODate);
        const int code = arrayInt(dailyCodes, index, 0);
        WeatherListModel::Entry entry;
        entry.dateText = date.toString(QStringLiteral("M月d日"));
        entry.weekdayText = index == 0 ? QStringLiteral("今天") : weekdayName(date);
        entry.condition = weatherDescription(code);
        entry.icon = weatherIconForCode(code);
        entry.highTemperature = arrayInt(dailyHighs, index, m_temperature);
        entry.lowTemperature = arrayInt(dailyLows, index, m_temperature);
        entry.precipitation = arrayInt(dailyPrecipitation, index, 0);
        entry.windSpeed = arrayInt(dailyWind, index, m_windSpeed);
        entry.weatherCode = code;
        dailyEntries.append(entry);
    }
    m_dailyModel.setEntries(std::move(dailyEntries));

    emit currentWeatherChanged();
    updateAdvice();
    return true;
}

void WeatherController::parseAirQuality(const QJsonObject &root)
{
    const QJsonObject current = root.value(QStringLiteral("current")).toObject();
    if (current.isEmpty())
        return;

    m_airQualityIndex = roundedJsonValue(current,
                                         QStringLiteral("us_aqi"),
                                         m_airQualityIndex);
    m_pm25 = current.value(QStringLiteral("pm2_5")).toDouble(m_pm25);
    m_pm10 = current.value(QStringLiteral("pm10")).toDouble(m_pm10);
    m_airQualityLevel = airQualityDescription(m_airQualityIndex);
    m_airQualityColor = airQualityColorForIndex(m_airQualityIndex);
    emit airQualityChanged();
    updateAdvice();
}

void WeatherController::updateAdvice()
{
    QString comfort;
    if (m_apparentTemperature >= 35)
        comfort = QStringLiteral("体感炎热，建议降低车内温度并及时补水");
    else if (m_apparentTemperature <= 5)
        comfort = QStringLiteral("体感寒冷，建议开启座椅加热和暖风");
    else if (m_humidity >= 85)
        comfort = QStringLiteral("湿度较高，建议开启除湿模式保持视野清晰");
    else if (m_airQualityIndex > 150)
        comfort = QStringLiteral("空气质量较差，建议关闭外循环并开启净化");
    else
        comfort = QStringLiteral("体感舒适，适合通风和短途出行");

    QString driving;
    if (m_weatherCode >= 95)
        driving = QStringLiteral("雷暴天气，请降低车速并避免临时停车在树下");
    else if (m_precipitationProbability >= 70)
        driving = QStringLiteral("降水概率较高，请提前开启雨刷并增加跟车距离");
    else if (m_visibility <= 3)
        driving = QStringLiteral("能见度较低，请开启雾灯并谨慎驾驶");
    else if (m_windSpeed >= 40)
        driving = QStringLiteral("侧风较强，经过桥梁和空旷路段时请稳住方向");
    else
        driving = QStringLiteral("道路状况良好，注意保持安全车距");

    if (m_comfortAdvice == comfort && m_drivingAdvice == driving)
        return;

    m_comfortAdvice = comfort;
    m_drivingAdvice = driving;
    emit adviceChanged();
}

void WeatherController::loadPreferences()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("weather"));
    m_locationQuery = settings.value(QStringLiteral("locationQuery"), m_locationQuery).toString();
    m_locationName = settings.value(QStringLiteral("locationName"), m_locationName).toString();
    m_latitude = settings.value(QStringLiteral("latitude"), m_latitude).toDouble();
    m_longitude = settings.value(QStringLiteral("longitude"), m_longitude).toDouble();
    m_locationMethod = settings.value(QStringLiteral("locationMethod"), m_locationMethod).toString();
    settings.endGroup();
}

void WeatherController::savePreferences() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("weather"));
    settings.setValue(QStringLiteral("locationQuery"), m_locationQuery);
    settings.setValue(QStringLiteral("locationName"), m_locationName);
    settings.setValue(QStringLiteral("latitude"), m_latitude);
    settings.setValue(QStringLiteral("longitude"), m_longitude);
    settings.setValue(QStringLiteral("locationMethod"), m_locationMethod);
    settings.endGroup();
}

bool WeatherController::loadCache()
{
    QFile file(cacheFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;

    const QJsonObject cache = document.object();
    const QJsonObject metadata = cache.value(QStringLiteral("metadata")).toObject();
    const QJsonObject forecast = cache.value(QStringLiteral("forecast")).toObject();
    if (forecast.isEmpty() || !parseForecast(forecast))
        return false;

    const QJsonObject airQuality = cache.value(QStringLiteral("airQuality")).toObject();
    if (!airQuality.isEmpty())
        parseAirQuality(airQuality);

    const QString cachedLocationName = metadata.value(QStringLiteral("locationName")).toString();
    if (!cachedLocationName.isEmpty() && cachedLocationName != m_locationName) {
        m_locationName = cachedLocationName;
        emit locationNameChanged();
    }

    m_lastForecastPayload = forecast;
    m_lastAirQualityPayload = airQuality;
    m_lastUpdated = metadata.value(QStringLiteral("savedAt")).toString();
    m_dataSource = QStringLiteral("本地缓存");
    m_usingCache = true;
    m_online = false;
    m_errorMessage.clear();
    emit statusChanged();
    return true;
}

void WeatherController::saveCache() const
{
    const QString filePath = cacheFilePath();
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QJsonObject metadata;
    metadata.insert(QStringLiteral("savedAt"), m_lastUpdated);
    metadata.insert(QStringLiteral("locationQuery"), m_locationQuery);
    metadata.insert(QStringLiteral("locationName"), m_locationName);
    metadata.insert(QStringLiteral("latitude"), m_latitude);
    metadata.insert(QStringLiteral("longitude"), m_longitude);

    QJsonObject cache;
    cache.insert(QStringLiteral("metadata"), metadata);
    cache.insert(QStringLiteral("forecast"), m_lastForecastPayload);
    cache.insert(QStringLiteral("airQuality"), m_lastAirQualityPayload);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    file.write(QJsonDocument(cache).toJson(QJsonDocument::Compact));
}

QString WeatherController::cacheFilePath() const
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(directory).filePath(QStringLiteral("weather_cache.json"));
}

void WeatherController::populateFallbackData()
{
    const QDateTime now = QDateTime::currentDateTime();
    m_temperature = 27;
    m_apparentTemperature = 29;
    m_humidity = 68;
    m_windSpeed = 13;
    m_windDirection = 120;
    m_visibility = 16;
    m_precipitationProbability = 25;
    m_weatherCode = 2;
    m_condition = weatherDescription(m_weatherCode);
    m_conditionIcon = weatherIconForCode(m_weatherCode);
    m_airQualityIndex = 42;
    m_airQualityLevel = airQualityDescription(m_airQualityIndex);
    m_airQualityColor = airQualityColorForIndex(m_airQualityIndex);
    m_pm25 = 18.0;
    m_pm10 = 31.0;
    m_lastUpdated = now.toString(QStringLiteral("MM-dd HH:mm"));

    QVector<WeatherListModel::Entry> hourlyEntries;
    hourlyEntries.reserve(kHourlyItemCount);
    for (int index = 0; index < kHourlyItemCount; ++index) {
        const QDateTime time = now.addSecs(index * 3600);
        const int code = index >= 7 && index <= 10 ? 61 : (index % 5 == 0 ? 3 : 2);
        WeatherListModel::Entry entry;
        entry.timeText = index == 0 ? QStringLiteral("现在")
                                    : time.time().toString(QStringLiteral("HH:mm"));
        entry.dateText = time.date().toString(QStringLiteral("M/d"));
        entry.condition = weatherDescription(code);
        entry.icon = weatherIconForCode(code);
        entry.temperature = 27 + qRound(3.0 * std::sin(index / 4.0));
        entry.precipitation = code == 61 ? 72 : 18 + (index * 7) % 23;
        entry.humidity = 62 + (index * 3) % 20;
        entry.windSpeed = 10 + (index * 2) % 12;
        entry.weatherCode = code;
        hourlyEntries.append(entry);
    }
    m_hourlyModel.setEntries(std::move(hourlyEntries));

    QVector<WeatherListModel::Entry> dailyEntries;
    dailyEntries.reserve(kDailyItemCount);
    const int codes[kDailyItemCount] = {2, 61, 3, 1, 0, 2, 80};
    for (int index = 0; index < kDailyItemCount; ++index) {
        const QDate date = now.date().addDays(index);
        const int code = codes[index];
        WeatherListModel::Entry entry;
        entry.dateText = date.toString(QStringLiteral("M月d日"));
        entry.weekdayText = index == 0 ? QStringLiteral("今天") : weekdayName(date);
        entry.condition = weatherDescription(code);
        entry.icon = weatherIconForCode(code);
        entry.highTemperature = 30 + (index % 3);
        entry.lowTemperature = 22 + (index % 2);
        entry.precipitation = code == 61 || code == 80 ? 76 : 18 + index * 4;
        entry.windSpeed = 12 + index;
        entry.weatherCode = code;
        dailyEntries.append(entry);
    }
    m_dailyModel.setEntries(std::move(dailyEntries));

    updateAdvice();
    emit currentWeatherChanged();
    emit airQualityChanged();
    emit statusChanged();
}

QString WeatherController::weatherDescription(int code)
{
    switch (code) {
    case 0:
        return QStringLiteral("晴");
    case 1:
    case 2:
        return QStringLiteral("多云");
    case 3:
        return QStringLiteral("阴");
    case 45:
    case 48:
        return QStringLiteral("雾");
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
        return QStringLiteral("毛毛雨");
    case 61:
        return QStringLiteral("小雨");
    case 63:
        return QStringLiteral("中雨");
    case 65:
    case 66:
    case 67:
        return QStringLiteral("大雨");
    case 71:
        return QStringLiteral("小雪");
    case 73:
        return QStringLiteral("中雪");
    case 75:
    case 77:
        return QStringLiteral("大雪");
    case 80:
    case 81:
    case 82:
        return QStringLiteral("阵雨");
    case 85:
    case 86:
        return QStringLiteral("阵雪");
    case 95:
    case 96:
    case 99:
        return QStringLiteral("雷暴");
    default:
        return QStringLiteral("天气未知");
    }
}

QString WeatherController::weatherIconForCode(int code)
{
    // 使用单色文本符号，避免 Windows 下批量彩色 Emoji 字形首次栅格化造成渲染峰值。
    if (code == 0)
        return QStringLiteral("☀");
    if (code == 1 || code == 2)
        return QStringLiteral("◒");
    if (code == 3)
        return QStringLiteral("☁");
    if (code == 45 || code == 48)
        return QStringLiteral("≋");
    if (code >= 51 && code <= 57)
        return QStringLiteral("⋰");
    if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82))
        return QStringLiteral("☂");
    if ((code >= 71 && code <= 77) || code == 85 || code == 86)
        return QStringLiteral("❄");
    if (code >= 95)
        return QStringLiteral("ϟ");
    return QStringLiteral("○");
}

QString WeatherController::airQualityDescription(int index)
{
    if (index <= 50)
        return QStringLiteral("优");
    if (index <= 100)
        return QStringLiteral("良");
    if (index <= 150)
        return QStringLiteral("轻度污染");
    if (index <= 200)
        return QStringLiteral("中度污染");
    if (index <= 300)
        return QStringLiteral("重度污染");
    return QStringLiteral("严重污染");
}

QString WeatherController::airQualityColorForIndex(int index)
{
    if (index <= 50)
        return QStringLiteral("#54D6A6");
    if (index <= 100)
        return QStringLiteral("#F1D16A");
    if (index <= 150)
        return QStringLiteral("#F3A45B");
    if (index <= 200)
        return QStringLiteral("#EF6B69");
    if (index <= 300)
        return QStringLiteral("#B47AE6");
    return QStringLiteral("#8C3F62");
}

QString WeatherController::windDirectionLabel(int degrees)
{
    static const QStringList directions = {
        QStringLiteral("北"),
        QStringLiteral("东北"),
        QStringLiteral("东"),
        QStringLiteral("东南"),
        QStringLiteral("南"),
        QStringLiteral("西南"),
        QStringLiteral("西"),
        QStringLiteral("西北")
    };
    const int normalized = ((degrees % 360) + 360) % 360;
    const int index = qRound(normalized / 45.0) % static_cast<int>(directions.size());
    return directions.at(index);
}

QString WeatherController::buildLocationName(const QJsonObject &result)
{
    QStringList parts;
    const QString name = result.value(QStringLiteral("name")).toString();
    const QString admin2 = result.value(QStringLiteral("admin2")).toString();
    const QString admin1 = result.value(QStringLiteral("admin1")).toString();

    for (const QString &part : {admin1, admin2, name}) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty() && !parts.contains(trimmed))
            parts.append(trimmed);
    }

    return parts.join(QLatin1Char(' '));
}

int WeatherController::roundedJsonValue(const QJsonObject &object,
                                        const QString &key,
                                        int fallback)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? qRound(value.toDouble()) : fallback;
}
