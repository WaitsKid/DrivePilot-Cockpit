#ifndef MAPCONTROLLER_H
#define MAPCONTROLLER_H

#include "MapSearchResultModel.h"
#include "NavigationStepModel.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>
#include <qqmlintegration.h>

class QJsonValue;

class MapController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(MapController)

    Q_PROPERTY(bool configured READ configured NOTIFY configuredChanged)
    Q_PROPERTY(QString amapJsKey READ amapJsKey NOTIFY configuredChanged)
    Q_PROPERTY(QString amapSecurityCode READ amapSecurityCode NOTIFY configuredChanged)
    Q_PROPERTY(QString defaultCity READ defaultCity NOTIFY configuredChanged)
    Q_PROPERTY(double defaultLongitude READ defaultLongitude NOTIFY configuredChanged)
    Q_PROPERTY(double defaultLatitude READ defaultLatitude NOTIFY configuredChanged)

    Q_PROPERTY(bool mapReady READ mapReady NOTIFY mapReadyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

    Q_PROPERTY(QAbstractItemModel *startSuggestions READ startSuggestions CONSTANT)
    Q_PROPERTY(QAbstractItemModel *endSuggestions READ endSuggestions CONSTANT)
    Q_PROPERTY(bool startSearching READ startSearching NOTIFY searchStateChanged)
    Q_PROPERTY(bool endSearching READ endSearching NOTIFY searchStateChanged)

    Q_PROPERTY(bool hasStart READ hasStart NOTIFY endpointsChanged)
    Q_PROPERTY(bool hasEnd READ hasEnd NOTIFY endpointsChanged)
    Q_PROPERTY(QString startName READ startName NOTIFY endpointsChanged)
    Q_PROPERTY(QString startAddress READ startAddress NOTIFY endpointsChanged)
    Q_PROPERTY(double startLongitude READ startLongitude NOTIFY endpointsChanged)
    Q_PROPERTY(double startLatitude READ startLatitude NOTIFY endpointsChanged)
    Q_PROPERTY(QString endName READ endName NOTIFY endpointsChanged)
    Q_PROPERTY(QString endAddress READ endAddress NOTIFY endpointsChanged)
    Q_PROPERTY(double endLongitude READ endLongitude NOTIFY endpointsChanged)
    Q_PROPERTY(double endLatitude READ endLatitude NOTIFY endpointsChanged)

    Q_PROPERTY(bool routeLoading READ routeLoading NOTIFY routeStateChanged)
    Q_PROPERTY(bool routeReady READ routeReady NOTIFY routeStateChanged)
    Q_PROPERTY(QString routeDistanceText READ routeDistanceText NOTIFY routeStateChanged)
    Q_PROPERTY(QString routeDurationText READ routeDurationText NOTIFY routeStateChanged)
    Q_PROPERTY(QString remainingDistanceText READ remainingDistanceText NOTIFY navigationStateChanged)
    Q_PROPERTY(QString remainingDurationText READ remainingDurationText NOTIFY navigationStateChanged)
    Q_PROPERTY(QString arrivalTimeText READ arrivalTimeText NOTIFY navigationStateChanged)
    Q_PROPERTY(QVariantList routePolyline READ routePolyline NOTIFY routeGeometryChanged)
    Q_PROPERTY(QAbstractItemModel *routeSteps READ routeSteps CONSTANT)

    Q_PROPERTY(bool navigating READ navigating NOTIFY navigationStateChanged)
    Q_PROPERTY(double vehicleLongitude READ vehicleLongitude NOTIFY vehiclePositionChanged)
    Q_PROPERTY(double vehicleLatitude READ vehicleLatitude NOTIFY vehiclePositionChanged)
    Q_PROPERTY(double vehicleHeading READ vehicleHeading NOTIFY vehiclePositionChanged)
    Q_PROPERTY(double simulatedSpeed READ simulatedSpeed NOTIFY vehiclePositionChanged)
    Q_PROPERTY(double simulationSpeedKmh READ simulationSpeedKmh WRITE setSimulationSpeedKmh NOTIFY simulationSpeedChanged)
    Q_PROPERTY(double routeProgress READ routeProgress NOTIFY navigationStateChanged)
    Q_PROPERTY(double remainingDistance READ remainingDistance NOTIFY navigationStateChanged)
    Q_PROPERTY(QString currentInstruction READ currentInstruction NOTIFY navigationInstructionChanged)
    Q_PROPERTY(QString nextRoadName READ nextRoadName NOTIFY navigationInstructionChanged)
    Q_PROPERTY(int currentStepIndex READ currentStepIndex NOTIFY navigationInstructionChanged)

public:
    explicit MapController(QObject *parent = nullptr);

    bool configured() const;
    QString amapJsKey() const;
    QString amapSecurityCode() const;
    QString defaultCity() const;
    double defaultLongitude() const;
    double defaultLatitude() const;

    bool mapReady() const;
    QString statusText() const;
    QString lastError() const;

    QAbstractItemModel *startSuggestions();
    QAbstractItemModel *endSuggestions();
    bool startSearching() const;
    bool endSearching() const;

    bool hasStart() const;
    bool hasEnd() const;
    QString startName() const;
    QString startAddress() const;
    double startLongitude() const;
    double startLatitude() const;
    QString endName() const;
    QString endAddress() const;
    double endLongitude() const;
    double endLatitude() const;

    bool routeLoading() const;
    bool routeReady() const;
    QString routeDistanceText() const;
    QString routeDurationText() const;
    QString remainingDistanceText() const;
    QString remainingDurationText() const;
    QString arrivalTimeText() const;
    QVariantList routePolyline() const;
    QAbstractItemModel *routeSteps();

    bool navigating() const;
    double vehicleLongitude() const;
    double vehicleLatitude() const;
    double vehicleHeading() const;
    double simulatedSpeed() const;
    double simulationSpeedKmh() const;
    double routeProgress() const;
    double remainingDistance() const;
    QString currentInstruction() const;
    QString nextRoadName() const;
    int currentStepIndex() const;

    Q_INVOKABLE void reloadConfiguration();
    Q_INVOKABLE void reportMapReady();
    Q_INVOKABLE void reportMapError(const QString &message);

    Q_INVOKABLE void beginSearch(int target, const QString &keyword);
    Q_INVOKABLE void receiveSearchResults(int target, const QString &json);
    Q_INVOKABLE void receiveSearchError(int target, const QString &message);
    Q_INVOKABLE void clearStartSuggestions();
    Q_INVOKABLE void clearEndSuggestions();
    Q_INVOKABLE void selectStartSuggestion(int row);
    Q_INVOKABLE void selectEndSuggestion(int row);
    Q_INVOKABLE void useCurrentLocationAsStart();
    Q_INVOKABLE void clearStart();
    Q_INVOKABLE void clearEnd();

    Q_INVOKABLE bool beginRoutePlanning();
    Q_INVOKABLE void receiveRouteResult(const QString &json);
    Q_INVOKABLE void receiveRouteError(const QString &message);
    Q_INVOKABLE void clearRoute();

    Q_INVOKABLE void startNavigation();
    Q_INVOKABLE void stopNavigation();
    Q_INVOKABLE void stopVehicleMotion();
    Q_INVOKABLE void simulateDrive(double meters);
    Q_INVOKABLE void setSimulationSpeedKmh(double speedKmh);

signals:
    void configuredChanged();
    void mapReadyChanged();
    void statusTextChanged();
    void lastErrorChanged();
    void searchStateChanged();
    void endpointsChanged();
    void routeStateChanged();
    void routeGeometryChanged();
    void navigationStateChanged();
    void vehiclePositionChanged();
    void navigationInstructionChanged();
    void simulationSpeedChanged();
    void toastRequested(const QString &message);

private:
    struct Endpoint {
        QString id;
        QString name;
        QString address;
        double longitude = 0.0;
        double latitude = 0.0;
        bool valid = false;
    };

    struct RoutePoint {
        double longitude = 0.0;
        double latitude = 0.0;
        double cumulativeMeters = 0.0;
        int stepIndex = 0;
    };

    enum class SearchTarget { Start = 1, End = 2 };

    void loadConfiguration();
    QString locateConfigPath() const;
    void setStatusText(const QString &text);
    void setLastError(const QString &text);
    void setSearchState(SearchTarget target, bool searching);
    void selectSuggestion(SearchTarget target, int row);
    void rebuildRouteVariants();
    void updateVehicleAtProgress(double progressMeters, double speedKmh);
    void updateNavigationInstruction();
    double remainingDurationSeconds() const;

    static QString normalizedSecret(QString value);
    static double jsonNumber(const QJsonValue &value, double fallback = 0.0);
    static QString jsonText(const QJsonValue &value);
    static QString formatDistance(double meters);
    static QString formatDuration(double seconds);
    static double haversineMeters(double longitude1,
                                  double latitude1,
                                  double longitude2,
                                  double latitude2);
    static double bearingDegrees(double longitude1,
                                 double latitude1,
                                 double longitude2,
                                 double latitude2);

    MapSearchResultModel m_startSuggestionModel;
    MapSearchResultModel m_endSuggestionModel;
    NavigationStepModel m_routeStepModel;

    QString m_amapJsKey;
    QString m_amapSecurityCode;
    QString m_defaultCity = QStringLiteral("杭州");
    double m_defaultLongitude = 120.15515;
    double m_defaultLatitude = 30.27415;

    bool m_mapReady = false;
    QString m_statusText = QStringLiteral("正在初始化导航地图");
    QString m_lastError;
    bool m_startSearching = false;
    bool m_endSearching = false;

    Endpoint m_start;
    Endpoint m_end;

    bool m_routeLoading = false;
    double m_routeDistanceMeters = 0.0;
    double m_routeDurationSeconds = 0.0;
    QVector<RoutePoint> m_routePoints;
    QVariantList m_routeVariants;

    bool m_navigating = false;
    double m_vehicleLongitude = 120.15515;
    double m_vehicleLatitude = 30.27415;
    double m_vehicleHeading = 0.0;
    double m_simulatedSpeed = 0.0;
    double m_simulationSpeedKmh = 1200.0;
    double m_routeProgressMeters = 0.0;
    QString m_currentInstruction;
    QString m_nextRoadName;
    int m_currentStepIndex = -1;
};

#endif
