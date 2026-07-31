#include "MapController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QTime>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr double kDefaultLongitude = 120.15515;
constexpr double kDefaultLatitude = 30.27415;
constexpr double kEarthRadiusMeters = 6371008.8;
constexpr double kPi = 3.14159265358979323846;
constexpr double kMinimumSimulationSpeed = 60.0;
constexpr double kMaximumSimulationSpeed = 3600.0;
constexpr int kMaximumRouteDisplayPoints = 1200;

bool validCoordinate(double longitude, double latitude)
{
    return std::isfinite(longitude) && std::isfinite(latitude)
        && longitude >= -180.0 && longitude <= 180.0
        && latitude >= -85.0 && latitude <= 85.0;
}
}

MapController::MapController(QObject *parent)
    : QObject(parent)
    , m_startSuggestionModel(this)
    , m_endSuggestionModel(this)
    , m_routeStepModel(this)
{
    loadConfiguration();
    useCurrentLocationAsStart();
    m_vehicleLongitude = m_start.longitude;
    m_vehicleLatitude = m_start.latitude;
}

bool MapController::configured() const
{
    return !m_amapJsKey.isEmpty() && !m_amapSecurityCode.isEmpty();
}

QString MapController::amapJsKey() const { return m_amapJsKey; }
QString MapController::amapSecurityCode() const { return m_amapSecurityCode; }
QString MapController::defaultCity() const { return m_defaultCity; }
double MapController::defaultLongitude() const { return m_defaultLongitude; }
double MapController::defaultLatitude() const { return m_defaultLatitude; }
bool MapController::mapReady() const { return m_mapReady; }
QString MapController::statusText() const { return m_statusText; }
QString MapController::lastError() const { return m_lastError; }
QAbstractItemModel *MapController::startSuggestions() { return &m_startSuggestionModel; }
QAbstractItemModel *MapController::endSuggestions() { return &m_endSuggestionModel; }
bool MapController::startSearching() const { return m_startSearching; }
bool MapController::endSearching() const { return m_endSearching; }
bool MapController::hasStart() const { return m_start.valid; }
bool MapController::hasEnd() const { return m_end.valid; }
QString MapController::startName() const { return m_start.name; }
QString MapController::startAddress() const { return m_start.address; }
double MapController::startLongitude() const { return m_start.longitude; }
double MapController::startLatitude() const { return m_start.latitude; }
QString MapController::endName() const { return m_end.name; }
QString MapController::endAddress() const { return m_end.address; }
double MapController::endLongitude() const { return m_end.longitude; }
double MapController::endLatitude() const { return m_end.latitude; }
bool MapController::routeLoading() const { return m_routeLoading; }
bool MapController::routeReady() const { return m_routePoints.size() >= 2; }
QString MapController::routeDistanceText() const { return formatDistance(m_routeDistanceMeters); }
QString MapController::routeDurationText() const { return formatDuration(m_routeDurationSeconds); }
QString MapController::remainingDistanceText() const { return formatDistance(remainingDistance()); }
QString MapController::remainingDurationText() const { return formatDuration(remainingDurationSeconds()); }

QString MapController::arrivalTimeText() const
{
    const qint64 seconds = qRound64(remainingDurationSeconds());
    return QDateTime::currentDateTime().addSecs(seconds).toString(QStringLiteral("HH:mm"));
}

QVariantList MapController::routePolyline() const { return m_routeVariants; }
QAbstractItemModel *MapController::routeSteps() { return &m_routeStepModel; }
bool MapController::navigating() const { return m_navigating; }
double MapController::vehicleLongitude() const { return m_vehicleLongitude; }
double MapController::vehicleLatitude() const { return m_vehicleLatitude; }
double MapController::vehicleHeading() const { return m_vehicleHeading; }
double MapController::simulatedSpeed() const { return m_simulatedSpeed; }
double MapController::simulationSpeedKmh() const { return m_simulationSpeedKmh; }

void MapController::setSimulationSpeedKmh(double speedKmh)
{
    const double normalized = qBound(kMinimumSimulationSpeed, speedKmh, kMaximumSimulationSpeed);
    if (qFuzzyCompare(m_simulationSpeedKmh, normalized))
        return;
    m_simulationSpeedKmh = normalized;
    emit simulationSpeedChanged();
}

double MapController::routeProgress() const
{
    if (!routeReady() || m_routeDistanceMeters <= 0.0)
        return 0.0;
    return qBound(0.0, m_routeProgressMeters / m_routeDistanceMeters, 1.0);
}

double MapController::remainingDistance() const
{
    return qMax(0.0, m_routeDistanceMeters - m_routeProgressMeters);
}

QString MapController::currentInstruction() const { return m_currentInstruction; }
QString MapController::nextRoadName() const { return m_nextRoadName; }
int MapController::currentStepIndex() const { return m_currentStepIndex; }

void MapController::reloadConfiguration()
{
    const QString oldKey = m_amapJsKey;
    const QString oldSecurityCode = m_amapSecurityCode;
    const QString oldCity = m_defaultCity;
    const double oldLongitude = m_defaultLongitude;
    const double oldLatitude = m_defaultLatitude;

    loadConfiguration();
    if (oldKey != m_amapJsKey
        || oldSecurityCode != m_amapSecurityCode
        || oldCity != m_defaultCity
        || !qFuzzyCompare(oldLongitude, m_defaultLongitude)
        || !qFuzzyCompare(oldLatitude, m_defaultLatitude)) {
        emit configuredChanged();
    }

    if (!m_start.valid || m_start.id == QStringLiteral("current-location"))
        useCurrentLocationAsStart();
}

void MapController::reportMapReady()
{
    if (!m_mapReady) {
        m_mapReady = true;
        emit mapReadyChanged();
    }
    setLastError({});
    setStatusText(QStringLiteral("高德导航地图已就绪"));
}

void MapController::reportMapError(const QString &message)
{
    if (m_mapReady) {
        m_mapReady = false;
        emit mapReadyChanged();
    }
    const QString error = message.trimmed().isEmpty()
        ? QStringLiteral("地图加载失败")
        : message.trimmed();
    setLastError(error);
    setStatusText(QStringLiteral("导航地图不可用"));
    emit toastRequested(error);
}

void MapController::beginSearch(int target, const QString &keyword)
{
    const SearchTarget searchTarget = target == static_cast<int>(SearchTarget::Start)
        ? SearchTarget::Start
        : SearchTarget::End;
    const QString trimmed = keyword.trimmed();
    if (trimmed.isEmpty()) {
        if (searchTarget == SearchTarget::Start)
            clearStartSuggestions();
        else
            clearEndSuggestions();
        return;
    }

    setSearchState(searchTarget, true);
    setLastError({});
    setStatusText(QStringLiteral("正在搜索“%1”").arg(trimmed));
}

void MapController::receiveSearchResults(int target, const QString &json)
{
    const SearchTarget searchTarget = target == static_cast<int>(SearchTarget::Start)
        ? SearchTarget::Start
        : SearchTarget::End;
    setSearchState(searchTarget, false);

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) {
        receiveSearchError(target, QStringLiteral("地点候选解析失败：%1").arg(error.errorString()));
        return;
    }

    QVector<MapPlaceResult> results;
    const QJsonArray array = document.array();
    results.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            continue;
        const QJsonObject object = value.toObject();
        MapPlaceResult result;
        result.id = jsonText(object.value(QStringLiteral("id")));
        result.name = jsonText(object.value(QStringLiteral("name"))).trimmed();
        result.district = jsonText(object.value(QStringLiteral("district"))).trimmed();
        result.address = jsonText(object.value(QStringLiteral("address"))).trimmed();
        result.longitude = jsonNumber(object.value(QStringLiteral("longitude")));
        result.latitude = jsonNumber(object.value(QStringLiteral("latitude")));
        result.distanceMeters = qMax(0.0, jsonNumber(object.value(QStringLiteral("distance"))));
        result.relevanceScore = qRound(jsonNumber(object.value(QStringLiteral("score"))));
        if (result.name.isEmpty() || !validCoordinate(result.longitude, result.latitude))
            continue;
        results.append(std::move(result));
    }

    std::stable_sort(results.begin(), results.end(), [](const MapPlaceResult &left, const MapPlaceResult &right) {
        if (left.relevanceScore != right.relevanceScore)
            return left.relevanceScore > right.relevanceScore;
        return left.distanceMeters < right.distanceMeters;
    });

    if (searchTarget == SearchTarget::Start)
        m_startSuggestionModel.setResults(std::move(results));
    else
        m_endSuggestionModel.setResults(std::move(results));

    const int count = searchTarget == SearchTarget::Start
        ? m_startSuggestionModel.rowCount()
        : m_endSuggestionModel.rowCount();
    setStatusText(count > 0
        ? QStringLiteral("找到 %1 个地点候选").arg(count)
        : QStringLiteral("没有找到可定位的地点"));
}

void MapController::receiveSearchError(int target, const QString &message)
{
    const SearchTarget searchTarget = target == static_cast<int>(SearchTarget::Start)
        ? SearchTarget::Start
        : SearchTarget::End;
    setSearchState(searchTarget, false);
    if (searchTarget == SearchTarget::Start)
        m_startSuggestionModel.clear();
    else
        m_endSuggestionModel.clear();
    setLastError(message);
    setStatusText(QStringLiteral("地点搜索失败"));
}

void MapController::clearStartSuggestions()
{
    m_startSuggestionModel.clear();
    setSearchState(SearchTarget::Start, false);
}

void MapController::clearEndSuggestions()
{
    m_endSuggestionModel.clear();
    setSearchState(SearchTarget::End, false);
}

void MapController::selectStartSuggestion(int row)
{
    selectSuggestion(SearchTarget::Start, row);
}

void MapController::selectEndSuggestion(int row)
{
    selectSuggestion(SearchTarget::End, row);
}

void MapController::useCurrentLocationAsStart()
{
    m_start.id = QStringLiteral("current-location");
    m_start.name = QStringLiteral("当前位置");
    m_start.address = m_defaultCity.isEmpty()
        ? QStringLiteral("模拟车辆当前位置")
        : QStringLiteral("%1 · 模拟车辆当前位置").arg(m_defaultCity);
    m_start.longitude = m_defaultLongitude;
    m_start.latitude = m_defaultLatitude;
    m_start.valid = true;
    m_vehicleLongitude = m_start.longitude;
    m_vehicleLatitude = m_start.latitude;
    clearStartSuggestions();
    clearRoute();
    emit endpointsChanged();
    emit vehiclePositionChanged();
    setStatusText(QStringLiteral("已使用当前位置作为起点"));
}

void MapController::clearStart()
{
    if (!m_start.valid)
        return;
    m_start = {};
    clearStartSuggestions();
    clearRoute();
    emit endpointsChanged();
}

void MapController::clearEnd()
{
    if (!m_end.valid)
        return;
    m_end = {};
    clearEndSuggestions();
    clearRoute();
    emit endpointsChanged();
}

bool MapController::beginRoutePlanning()
{
    if (!m_mapReady) {
        emit toastRequested(QStringLiteral("地图尚未加载完成"));
        return false;
    }
    if (!m_start.valid || !m_end.valid) {
        emit toastRequested(QStringLiteral("请先选择起点和终点"));
        return false;
    }
    if (haversineMeters(m_start.longitude, m_start.latitude,
                        m_end.longitude, m_end.latitude) < 5.0) {
        emit toastRequested(QStringLiteral("起点和终点距离过近"));
        return false;
    }

    clearRoute();
    m_routeLoading = true;
    setLastError({});
    setStatusText(QStringLiteral("正在规划驾车路线"));
    emit routeStateChanged();
    return true;
}

void MapController::receiveRouteResult(const QString &json)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        receiveRouteError(QStringLiteral("路线结果解析失败：%1").arg(error.errorString()));
        return;
    }

    const QJsonObject root = document.object();
    const QJsonArray stepsArray = root.value(QStringLiteral("steps")).toArray();
    QVector<NavigationStep> steps;
    QVector<RoutePoint> points;
    steps.reserve(stepsArray.size());

    double cumulative = 0.0;
    for (int stepIndex = 0; stepIndex < stepsArray.size(); ++stepIndex) {
        const QJsonObject stepObject = stepsArray.at(stepIndex).toObject();
        NavigationStep step;
        step.instruction = jsonText(stepObject.value(QStringLiteral("instruction"))).trimmed();
        step.roadName = jsonText(stepObject.value(QStringLiteral("road"))).trimmed();
        step.action = jsonText(stepObject.value(QStringLiteral("action"))).trimmed();
        step.cumulativeStart = cumulative;

        const QJsonArray pathArray = stepObject.value(QStringLiteral("path")).toArray();
        for (const QJsonValue &pathValue : pathArray) {
            const QJsonArray coordinate = pathValue.toArray();
            if (coordinate.size() < 2)
                continue;
            const double longitude = jsonNumber(coordinate.at(0));
            const double latitude = jsonNumber(coordinate.at(1));
            if (!validCoordinate(longitude, latitude))
                continue;

            if (!points.isEmpty()) {
                const RoutePoint &previous = points.constLast();
                const double segment = haversineMeters(previous.longitude,
                                                       previous.latitude,
                                                       longitude,
                                                       latitude);
                if (segment < 0.03)
                    continue;
                cumulative += segment;
            }
            points.append({longitude, latitude, cumulative, stepIndex});
        }

        const double reportedDistance = qMax(0.0, jsonNumber(stepObject.value(QStringLiteral("distance"))));
        step.distanceMeters = reportedDistance > 0.0
            ? reportedDistance
            : qMax(0.0, cumulative - step.cumulativeStart);
        step.cumulativeEnd = cumulative;
        if (step.instruction.isEmpty())
            step.instruction = step.roadName.isEmpty()
                ? QStringLiteral("沿当前道路继续行驶")
                : QStringLiteral("沿%1行驶").arg(step.roadName);
        steps.append(std::move(step));
    }

    if (points.size() < 2) {
        receiveRouteError(QStringLiteral("高德未返回可模拟的路线轨迹"));
        return;
    }

    m_routePoints = std::move(points);
    m_routeDistanceMeters = cumulative;
    m_routeDurationSeconds = qMax(1.0, jsonNumber(root.value(QStringLiteral("duration")), 1.0));
    m_routeStepModel.setSteps(std::move(steps));
    m_routeLoading = false;
    m_routeProgressMeters = 0.0;
    m_navigating = false;
    m_simulatedSpeed = 0.0;
    m_currentStepIndex = -1;
    rebuildRouteVariants();
    updateVehicleAtProgress(0.0, 0.0);
    setLastError({});
    setStatusText(QStringLiteral("路线规划完成，可开始模拟导航"));
    emit routeStateChanged();
    emit routeGeometryChanged();
    emit navigationStateChanged();
    emit toastRequested(QStringLiteral("路线规划完成"));
}

void MapController::receiveRouteError(const QString &message)
{
    m_routeLoading = false;
    setLastError(message.trimmed().isEmpty() ? QStringLiteral("路线规划失败") : message.trimmed());
    setStatusText(QStringLiteral("路线规划失败"));
    emit routeStateChanged();
    emit toastRequested(m_lastError);
}

void MapController::clearRoute()
{
    const bool hadRoute = routeReady() || m_routeLoading || m_navigating;
    m_routeLoading = false;
    m_routeDistanceMeters = 0.0;
    m_routeDurationSeconds = 0.0;
    m_routePoints.clear();
    m_routeVariants.clear();
    m_routeStepModel.clear();
    m_navigating = false;
    m_routeProgressMeters = 0.0;
    m_simulatedSpeed = 0.0;
    m_currentInstruction.clear();
    m_nextRoadName.clear();
    m_currentStepIndex = -1;
    if (m_start.valid) {
        m_vehicleLongitude = m_start.longitude;
        m_vehicleLatitude = m_start.latitude;
        m_vehicleHeading = 0.0;
    }
    if (hadRoute) {
        emit routeStateChanged();
        emit routeGeometryChanged();
        emit navigationStateChanged();
        emit navigationInstructionChanged();
        emit vehiclePositionChanged();
    }
}

void MapController::startNavigation()
{
    if (!routeReady()) {
        emit toastRequested(QStringLiteral("请先完成路线规划"));
        return;
    }

    if (m_routeProgressMeters >= m_routeDistanceMeters - 0.1)
        m_routeProgressMeters = 0.0;
    m_navigating = true;
    updateVehicleAtProgress(m_routeProgressMeters, 0.0);
    setStatusText(QStringLiteral("模拟导航中：按住 W 前进，S 后退"));
    emit navigationStateChanged();
    emit toastRequested(QStringLiteral("模拟导航已开始"));
}

void MapController::stopNavigation()
{
    if (!m_navigating && qFuzzyIsNull(m_simulatedSpeed))
        return;
    m_navigating = false;
    m_simulatedSpeed = 0.0;
    setStatusText(QStringLiteral("模拟导航已暂停"));
    emit vehiclePositionChanged();
    emit navigationStateChanged();
}

void MapController::stopVehicleMotion()
{
    if (qFuzzyIsNull(m_simulatedSpeed))
        return;
    m_simulatedSpeed = 0.0;
    emit vehiclePositionChanged();
}

void MapController::simulateDrive(double meters)
{
    if (!m_navigating || !routeReady() || qFuzzyIsNull(meters))
        return;

    const double nextProgress = qBound(0.0,
                                       m_routeProgressMeters + meters,
                                       m_routeDistanceMeters);
    const double signedSpeed = meters > 0.0 ? m_simulationSpeedKmh : -m_simulationSpeedKmh;
    updateVehicleAtProgress(nextProgress, signedSpeed);

    if (nextProgress >= m_routeDistanceMeters - 0.05) {
        m_navigating = false;
        m_simulatedSpeed = 0.0;
        m_currentInstruction = QStringLiteral("已到达目的地：%1").arg(m_end.name);
        m_nextRoadName.clear();
        setStatusText(QStringLiteral("已到达目的地"));
        emit vehiclePositionChanged();
        emit navigationInstructionChanged();
        emit navigationStateChanged();
        emit toastRequested(QStringLiteral("已到达目的地"));
    }
}

void MapController::loadConfiguration()
{
    m_amapJsKey.clear();
    m_amapSecurityCode.clear();
    m_defaultCity = QStringLiteral("杭州");
    m_defaultLongitude = kDefaultLongitude;
    m_defaultLatitude = kDefaultLatitude;

    const QString path = locateConfigPath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setLastError(QStringLiteral("未找到 config.json"));
        setStatusText(QStringLiteral("请在运行目录配置高德 JS API"));
        return;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        setLastError(QStringLiteral("config.json 格式错误：%1").arg(error.errorString()));
        setStatusText(QStringLiteral("导航配置读取失败"));
        return;
    }

    const QJsonObject object = document.object();
    m_amapJsKey = normalizedSecret(object.value(QStringLiteral("amap_js_key")).toString());
    m_amapSecurityCode = normalizedSecret(object.value(QStringLiteral("amap_js_security_code")).toString());
    const QString configuredCity = object.value(QStringLiteral("amap_default_city")).toString().trimmed();
    if (!configuredCity.isEmpty())
        m_defaultCity = configuredCity;

    const double longitude = jsonNumber(object.value(QStringLiteral("amap_default_longitude")), kDefaultLongitude);
    const double latitude = jsonNumber(object.value(QStringLiteral("amap_default_latitude")), kDefaultLatitude);
    if (validCoordinate(longitude, latitude)) {
        m_defaultLongitude = longitude;
        m_defaultLatitude = latitude;
    }

    if (configured()) {
        setLastError({});
        setStatusText(QStringLiteral("高德 JS API 配置已读取"));
    } else {
        setLastError(QStringLiteral("config.json 缺少 amap_js_key 或 amap_js_security_code"));
        setStatusText(QStringLiteral("等待配置高德 JS API"));
    }
}

QString MapController::locateConfigPath() const
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

void MapController::setStatusText(const QString &text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void MapController::setLastError(const QString &text)
{
    if (m_lastError == text)
        return;
    m_lastError = text;
    emit lastErrorChanged();
}

void MapController::setSearchState(SearchTarget target, bool searching)
{
    bool *state = target == SearchTarget::Start ? &m_startSearching : &m_endSearching;
    if (*state == searching)
        return;
    *state = searching;
    emit searchStateChanged();
}

void MapController::selectSuggestion(SearchTarget target, int row)
{
    const MapPlaceResult *place = target == SearchTarget::Start
        ? m_startSuggestionModel.placeAt(row)
        : m_endSuggestionModel.placeAt(row);
    if (!place)
        return;

    Endpoint endpoint;
    endpoint.id = place->id;
    endpoint.name = place->name;
    endpoint.address = !place->address.isEmpty() ? place->address : place->district;
    endpoint.longitude = place->longitude;
    endpoint.latitude = place->latitude;
    endpoint.valid = true;

    clearRoute();
    if (target == SearchTarget::Start) {
        m_start = std::move(endpoint);
        m_startSuggestionModel.clear();
        m_vehicleLongitude = m_start.longitude;
        m_vehicleLatitude = m_start.latitude;
        m_vehicleHeading = 0.0;
        emit vehiclePositionChanged();
    } else {
        m_end = std::move(endpoint);
        m_endSuggestionModel.clear();
    }
    emit endpointsChanged();
    setStatusText(QStringLiteral("已选择%1：%2")
        .arg(target == SearchTarget::Start ? QStringLiteral("起点") : QStringLiteral("终点"),
             target == SearchTarget::Start ? m_start.name : m_end.name));
}

void MapController::rebuildRouteVariants()
{
    m_routeVariants.clear();
    if (m_routePoints.isEmpty())
        return;

    const int count = m_routePoints.size();
    const double stride = count > kMaximumRouteDisplayPoints
        ? double(count - 1) / double(kMaximumRouteDisplayPoints - 1)
        : 1.0;
    const int outputCount = count > kMaximumRouteDisplayPoints
        ? kMaximumRouteDisplayPoints
        : count;
    m_routeVariants.reserve(outputCount);

    int previousIndex = -1;
    for (int outputIndex = 0; outputIndex < outputCount; ++outputIndex) {
        const int sourceIndex = outputIndex == outputCount - 1
            ? count - 1
            : qBound(0, qRound(outputIndex * stride), count - 1);
        if (sourceIndex == previousIndex)
            continue;
        previousIndex = sourceIndex;
        const RoutePoint &point = m_routePoints.at(sourceIndex);
        QVariantMap value;
        value.insert(QStringLiteral("longitude"), point.longitude);
        value.insert(QStringLiteral("latitude"), point.latitude);
        value.insert(QStringLiteral("distance"), point.cumulativeMeters);
        value.insert(QStringLiteral("stepIndex"), point.stepIndex);
        m_routeVariants.append(value);
    }
}

void MapController::updateVehicleAtProgress(double progressMeters, double speedKmh)
{
    if (!routeReady())
        return;

    m_routeProgressMeters = qBound(0.0, progressMeters, m_routeDistanceMeters);
    auto upper = std::lower_bound(m_routePoints.constBegin(),
                                  m_routePoints.constEnd(),
                                  m_routeProgressMeters,
                                  [](const RoutePoint &point, double distance) {
                                      return point.cumulativeMeters < distance;
                                  });

    int upperIndex = static_cast<int>(std::distance(m_routePoints.constBegin(), upper));
    if (upperIndex <= 0)
        upperIndex = 1;
    if (upperIndex >= m_routePoints.size())
        upperIndex = m_routePoints.size() - 1;
    const int lowerIndex = upperIndex - 1;
    const RoutePoint &lower = m_routePoints.at(lowerIndex);
    const RoutePoint &higher = m_routePoints.at(upperIndex);
    const double segmentLength = qMax(0.0001, higher.cumulativeMeters - lower.cumulativeMeters);
    const double ratio = qBound(0.0,
                                (m_routeProgressMeters - lower.cumulativeMeters) / segmentLength,
                                1.0);

    m_vehicleLongitude = lower.longitude + (higher.longitude - lower.longitude) * ratio;
    m_vehicleLatitude = lower.latitude + (higher.latitude - lower.latitude) * ratio;
    m_vehicleHeading = bearingDegrees(lower.longitude,
                                      lower.latitude,
                                      higher.longitude,
                                      higher.latitude);
    m_simulatedSpeed = speedKmh;
    m_currentStepIndex = ratio < 0.5 ? lower.stepIndex : higher.stepIndex;
    updateNavigationInstruction();
    emit vehiclePositionChanged();
    emit navigationStateChanged();
}

void MapController::updateNavigationInstruction()
{
    const NavigationStep *current = m_routeStepModel.stepAt(m_currentStepIndex);
    const NavigationStep *next = m_routeStepModel.stepAt(m_currentStepIndex + 1);
    if (!current) {
        m_currentInstruction = QStringLiteral("沿规划路线行驶");
        m_nextRoadName.clear();
        m_routeStepModel.setActiveIndex(-1);
    } else {
        m_currentInstruction = current->instruction;
        m_nextRoadName = next && !next->roadName.isEmpty()
            ? next->roadName
            : m_end.name;
        m_routeStepModel.setActiveIndex(m_currentStepIndex);
    }
    emit navigationInstructionChanged();
}

double MapController::remainingDurationSeconds() const
{
    if (!routeReady() || m_routeDistanceMeters <= 0.0)
        return 0.0;
    return m_routeDurationSeconds * qBound(0.0, remainingDistance() / m_routeDistanceMeters, 1.0);
}

QString MapController::normalizedSecret(QString value)
{
    value = value.trimmed();
    const QString lowered = value.toLower();
    if (value.isEmpty()
        || value.contains(QStringLiteral("你的"))
        || value.contains(QStringLiteral("填写"))
        || value.contains(QStringLiteral("填入"))
        || lowered.contains(QStringLiteral("replace_me"))
        || lowered.contains(QStringLiteral("your_"))) {
        return {};
    }
    return value;
}

double MapController::jsonNumber(const QJsonValue &value, double fallback)
{
    if (value.isDouble())
        return value.toDouble(fallback);
    if (value.isString()) {
        bool ok = false;
        const double number = value.toString().trimmed().toDouble(&ok);
        if (ok)
            return number;
    }
    return fallback;
}

QString MapController::jsonText(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();
    if (value.isDouble())
        return QString::number(value.toDouble());
    if (value.isArray()) {
        QStringList parts;
        for (const QJsonValue &item : value.toArray()) {
            const QString text = jsonText(item).trimmed();
            if (!text.isEmpty())
                parts.append(text);
        }
        return parts.join(QStringLiteral("、"));
    }
    return {};
}

QString MapController::formatDistance(double meters)
{
    if (!std::isfinite(meters) || meters <= 0.0)
        return QStringLiteral("0 m");
    if (meters < 1000.0)
        return QStringLiteral("%1 m").arg(qRound(meters));
    return QStringLiteral("%1 km").arg(meters / 1000.0, 0, 'f', meters < 10000.0 ? 1 : 0);
}

QString MapController::formatDuration(double seconds)
{
    if (!std::isfinite(seconds) || seconds <= 0.0)
        return QStringLiteral("0 分钟");
    const int minutes = qMax(1, qRound(seconds / 60.0));
    if (minutes < 60)
        return QStringLiteral("%1 分钟").arg(minutes);
    return QStringLiteral("%1 小时 %2 分钟").arg(minutes / 60).arg(minutes % 60);
}

double MapController::haversineMeters(double longitude1,
                                      double latitude1,
                                      double longitude2,
                                      double latitude2)
{
    const double lat1 = qDegreesToRadians(latitude1);
    const double lat2 = qDegreesToRadians(latitude2);
    const double deltaLat = lat2 - lat1;
    const double deltaLon = qDegreesToRadians(longitude2 - longitude1);
    const double rawA = std::sin(deltaLat / 2.0) * std::sin(deltaLat / 2.0)
        + std::cos(lat1) * std::cos(lat2)
            * std::sin(deltaLon / 2.0) * std::sin(deltaLon / 2.0);
    const double a = qBound(0.0, rawA, 1.0);
    return 2.0 * kEarthRadiusMeters * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

double MapController::bearingDegrees(double longitude1,
                                     double latitude1,
                                     double longitude2,
                                     double latitude2)
{
    const double lat1 = qDegreesToRadians(latitude1);
    const double lat2 = qDegreesToRadians(latitude2);
    const double deltaLon = qDegreesToRadians(longitude2 - longitude1);
    const double y = std::sin(deltaLon) * std::cos(lat2);
    const double x = std::cos(lat1) * std::sin(lat2)
        - std::sin(lat1) * std::cos(lat2) * std::cos(deltaLon);
    double angle = qRadiansToDegrees(std::atan2(y, x));
    if (angle < 0.0)
        angle += 360.0;
    return angle;
}
