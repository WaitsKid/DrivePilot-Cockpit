#ifndef VEHICLECONTROLLER_H
#define VEHICLECONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <qqmlintegration.h>

class VehicleController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(VehicleData)

    Q_PROPERTY(int MODE_ECO READ getMODE_ECO CONSTANT)
    Q_PROPERTY(int MODE_NORMAL READ getMODE_NORMAL CONSTANT)
    Q_PROPERTY(int MODE_SPORT READ getMODE_SPORT CONSTANT)

    Q_PROPERTY(int FLOW_IDLE READ getFLOW_IDLE CONSTANT)
    Q_PROPERTY(int FLOW_DRIVE READ getFLOW_DRIVE CONSTANT)
    Q_PROPERTY(int FLOW_REGEN READ getFLOW_REGEN CONSTANT)

    Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY batteryLevelChanged)
    Q_PROPERTY(int remainingRange READ remainingRange NOTIFY remainingRangeChanged)
    Q_PROPERTY(int speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(double instantPower READ instantPower NOTIFY instantPowerChanged)
    Q_PROPERTY(double averageConsumption READ averageConsumption NOTIFY averageConsumptionChanged)
    Q_PROPERTY(double batteryTemperature READ batteryTemperature NOTIFY batteryTemperatureChanged)
    Q_PROPERTY(double motorTemperature READ motorTemperature NOTIFY motorTemperatureChanged)

    Q_PROPERTY(double frontLeftPressure READ frontLeftPressure NOTIFY tirePressureChanged)
    Q_PROPERTY(double frontRightPressure READ frontRightPressure NOTIFY tirePressureChanged)
    Q_PROPERTY(double rearLeftPressure READ rearLeftPressure NOTIFY tirePressureChanged)
    Q_PROPERTY(double rearRightPressure READ rearRightPressure NOTIFY tirePressureChanged)

    Q_PROPERTY(int driveMode READ driveMode WRITE setDriveMode NOTIFY driveModeChanged)
    Q_PROPERTY(int energyFlowState READ energyFlowState NOTIFY energyFlowStateChanged)
    Q_PROPERTY(bool tireFaultActive READ tireFaultActive NOTIFY tireFaultActiveChanged)
    Q_PROPERTY(bool simulationRunning READ simulationRunning WRITE setSimulationRunning NOTIFY simulationRunningChanged)
    Q_PROPERTY(QString healthText READ healthText NOTIFY healthTextChanged)
    Q_PROPERTY(QString driveModeText READ driveModeText NOTIFY driveModeChanged)

public:
    explicit VehicleController(QObject *parent = nullptr);

    static constexpr int MODE_ECO = 0;
    static constexpr int MODE_NORMAL = 1;
    static constexpr int MODE_SPORT = 2;

    static constexpr int FLOW_IDLE = 0;
    static constexpr int FLOW_DRIVE = 1;
    static constexpr int FLOW_REGEN = 2;

    int getMODE_ECO() const;
    int getMODE_NORMAL() const;
    int getMODE_SPORT() const;

    int getFLOW_IDLE() const;
    int getFLOW_DRIVE() const;
    int getFLOW_REGEN() const;

    int batteryLevel() const;
    int remainingRange() const;
    int speed() const;
    double instantPower() const;
    double averageConsumption() const;
    double batteryTemperature() const;
    double motorTemperature() const;

    double frontLeftPressure() const;
    double frontRightPressure() const;
    double rearLeftPressure() const;
    double rearRightPressure() const;

    int driveMode() const;
    void setDriveMode(int mode);

    int energyFlowState() const;
    bool tireFaultActive() const;

    bool simulationRunning() const;
    void setSimulationRunning(bool running);

    QString healthText() const;
    QString driveModeText() const;

    Q_INVOKABLE void simulateTireFault();
    Q_INVOKABLE void clearTireFault();
    Q_INVOKABLE void resetSimulation();

signals:
    void batteryLevelChanged();
    void remainingRangeChanged();
    void speedChanged();
    void instantPowerChanged();
    void averageConsumptionChanged();
    void batteryTemperatureChanged();
    void motorTemperatureChanged();
    void tirePressureChanged();
    void driveModeChanged();
    void energyFlowStateChanged();
    void tireFaultActiveChanged();
    void simulationRunningChanged();
    void healthTextChanged();

private slots:
    void updateSimulation();

private:
    void updateRemainingRange();
    void setEnergyFlowState(int state);
    void setSpeed(int value);
    void setInstantPower(double value);
    void setAverageConsumption(double value);
    void setBatteryTemperature(double value);
    void setMotorTemperature(double value);
    void setBatteryLevel(int value);
    void setTirePressures(double frontLeft,
                          double frontRight,
                          double rearLeft,
                          double rearRight);

    static double randomBetween(double minimum, double maximum);
    static double roundedTenths(double value);
    static double roundedHundredths(double value);

    QTimer m_simulationTimer;
    int m_tickCount = 0;

    int m_batteryLevel = 78;
    int m_remainingRange = 506;
    int m_speed = 46;
    double m_instantPower = 17.6;
    double m_averageConsumption = 14.3;
    double m_batteryTemperature = 31.0;
    double m_motorTemperature = 47.0;

    double m_frontLeftPressure = 2.42;
    double m_frontRightPressure = 2.40;
    double m_rearLeftPressure = 2.45;
    double m_rearRightPressure = 2.43;

    int m_driveMode = MODE_NORMAL;
    int m_energyFlowState = FLOW_DRIVE;
    bool m_tireFaultActive = false;
    bool m_simulationRunning = true;
};

#endif
