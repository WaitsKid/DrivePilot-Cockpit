#include "VehicleController.h"

#include <QRandomGenerator>
#include <QSettings>
#include <QtGlobal>
#include <cmath>

VehicleController::VehicleController(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    m_driveMode = qBound(MODE_ECO,
                         settings.value(QStringLiteral("vehicle/driveMode"),
                                        MODE_NORMAL).toInt(),
                         MODE_SPORT);
    updateRemainingRange();

    m_simulationTimer.setInterval(1000);
    m_simulationTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_simulationTimer,
            &QTimer::timeout,
            this,
            &VehicleController::updateSimulation);
    m_simulationTimer.start();
}

int VehicleController::getMODE_ECO() const
{
    return MODE_ECO;
}

int VehicleController::getMODE_NORMAL() const
{
    return MODE_NORMAL;
}

int VehicleController::getMODE_SPORT() const
{
    return MODE_SPORT;
}

int VehicleController::getFLOW_IDLE() const
{
    return FLOW_IDLE;
}

int VehicleController::getFLOW_DRIVE() const
{
    return FLOW_DRIVE;
}

int VehicleController::getFLOW_REGEN() const
{
    return FLOW_REGEN;
}

int VehicleController::batteryLevel() const
{
    return m_batteryLevel;
}

int VehicleController::remainingRange() const
{
    return m_remainingRange;
}

int VehicleController::speed() const
{
    return m_speed;
}

double VehicleController::instantPower() const
{
    return m_instantPower;
}

double VehicleController::averageConsumption() const
{
    return m_averageConsumption;
}

double VehicleController::batteryTemperature() const
{
    return m_batteryTemperature;
}

double VehicleController::motorTemperature() const
{
    return m_motorTemperature;
}

double VehicleController::frontLeftPressure() const
{
    return m_frontLeftPressure;
}

double VehicleController::frontRightPressure() const
{
    return m_frontRightPressure;
}

double VehicleController::rearLeftPressure() const
{
    return m_rearLeftPressure;
}

double VehicleController::rearRightPressure() const
{
    return m_rearRightPressure;
}

int VehicleController::driveMode() const
{
    return m_driveMode;
}

void VehicleController::setDriveMode(int mode)
{
    const int boundedMode = qBound(MODE_ECO, mode, MODE_SPORT);
    if (m_driveMode == boundedMode)
        return;

    m_driveMode = boundedMode;
    emit driveModeChanged();
    updateRemainingRange();

    QSettings settings;
    settings.setValue(QStringLiteral("vehicle/driveMode"), m_driveMode);
}

int VehicleController::energyFlowState() const
{
    return m_energyFlowState;
}

bool VehicleController::tireFaultActive() const
{
    return m_tireFaultActive;
}

bool VehicleController::simulationRunning() const
{
    return m_simulationRunning;
}

void VehicleController::setSimulationRunning(bool running)
{
    if (m_simulationRunning == running)
        return;

    m_simulationRunning = running;
    emit simulationRunningChanged();

    if (m_simulationRunning) {
        m_simulationTimer.start();
    } else {
        m_simulationTimer.stop();
        setSpeed(0);
        setInstantPower(0.0);
        setEnergyFlowState(FLOW_IDLE);
    }
}

QString VehicleController::healthText() const
{
    return m_tireFaultActive ? QStringLiteral("胎压异常") : QStringLiteral("车况良好");
}

QString VehicleController::driveModeText() const
{
    switch (m_driveMode) {
    case MODE_ECO:
        return QStringLiteral("经济模式");
    case MODE_SPORT:
        return QStringLiteral("运动模式");
    case MODE_NORMAL:
    default:
        return QStringLiteral("标准模式");
    }
}

void VehicleController::simulateTireFault()
{
    if (m_tireFaultActive)
        return;

    m_tireFaultActive = true;
    m_frontRightPressure = 1.72;
    emit tirePressureChanged();
    emit tireFaultActiveChanged();
    emit healthTextChanged();
}

void VehicleController::clearTireFault()
{
    if (!m_tireFaultActive)
        return;

    m_tireFaultActive = false;
    m_frontRightPressure = 2.40;
    emit tirePressureChanged();
    emit tireFaultActiveChanged();
    emit healthTextChanged();
}

void VehicleController::resetSimulation()
{
    m_tickCount = 0;
    setBatteryLevel(78);
    setSpeed(46);
    setInstantPower(17.6);
    setAverageConsumption(14.3);
    setBatteryTemperature(31.0);
    setMotorTemperature(47.0);
    setTirePressures(2.42, 2.40, 2.45, 2.43);

    if (m_driveMode != MODE_NORMAL) {
        m_driveMode = MODE_NORMAL;
        emit driveModeChanged();

        QSettings settings;
        settings.setValue(QStringLiteral("vehicle/driveMode"), m_driveMode);
    }

    if (m_tireFaultActive) {
        m_tireFaultActive = false;
        emit tireFaultActiveChanged();
        emit healthTextChanged();
    }

    if (!m_simulationRunning) {
        m_simulationRunning = true;
        emit simulationRunningChanged();
        m_simulationTimer.start();
    }

    setEnergyFlowState(FLOW_DRIVE);
    updateRemainingRange();
}

void VehicleController::updateSimulation()
{
    if (!m_simulationRunning)
        return;

    ++m_tickCount;

    int targetSpeed = 48;
    int maximumSpeed = 88;
    double baseConsumption = 13.2;
    double powerMultiplier = 0.82;

    if (m_driveMode == MODE_NORMAL) {
        targetSpeed = 58;
        maximumSpeed = 108;
        baseConsumption = 14.6;
        powerMultiplier = 1.0;
    } else if (m_driveMode == MODE_SPORT) {
        targetSpeed = 72;
        maximumSpeed = 132;
        baseConsumption = 17.4;
        powerMultiplier = 1.28;
    }

    const int speedDirection = m_speed < targetSpeed ? 1 : -1;
    const int speedDelta = QRandomGenerator::global()->bounded(1, 6) * speedDirection
                           + QRandomGenerator::global()->bounded(-2, 3);
    setSpeed(qBound(18, m_speed + speedDelta, maximumSpeed));

    const bool regenerativeBraking = QRandomGenerator::global()->bounded(100) < 18;
    if (regenerativeBraking) {
        setInstantPower(-randomBetween(5.0, 19.0));
        setEnergyFlowState(FLOW_REGEN);
    } else {
        const double speedPower = 6.0 + static_cast<double>(m_speed) * 0.23;
        setInstantPower(randomBetween(speedPower * 0.78,
                                      speedPower * 1.18) * powerMultiplier);
        setEnergyFlowState(FLOW_DRIVE);
    }

    const double targetConsumption = baseConsumption
                                     + static_cast<double>(m_speed - targetSpeed) * 0.025;
    setAverageConsumption(m_averageConsumption
                          + (targetConsumption - m_averageConsumption) * 0.18
                          + randomBetween(-0.18, 0.18));

    const double batteryTarget = 28.0 + std::abs(m_instantPower) * 0.12;
    const double motorTarget = 39.0 + std::abs(m_instantPower) * 0.48;
    setBatteryTemperature(m_batteryTemperature
                          + (batteryTarget - m_batteryTemperature) * 0.16
                          + randomBetween(-0.20, 0.20));
    setMotorTemperature(m_motorTemperature
                        + (motorTarget - m_motorTemperature) * 0.18
                        + randomBetween(-0.35, 0.35));

    if (!m_tireFaultActive) {
        setTirePressures(m_frontLeftPressure + randomBetween(-0.008, 0.008),
                         m_frontRightPressure + randomBetween(-0.008, 0.008),
                         m_rearLeftPressure + randomBetween(-0.008, 0.008),
                         m_rearRightPressure + randomBetween(-0.008, 0.008));
    }

    if (m_tickCount % 24 == 0 && m_batteryLevel > 12)
        setBatteryLevel(m_batteryLevel - 1);
}

void VehicleController::updateRemainingRange()
{
    double kilometresPerPercent = 6.49;
    if (m_driveMode == MODE_ECO)
        kilometresPerPercent = 6.85;
    else if (m_driveMode == MODE_SPORT)
        kilometresPerPercent = 5.85;

    const int newRange = qRound(static_cast<double>(m_batteryLevel)
                                * kilometresPerPercent);
    if (m_remainingRange == newRange)
        return;

    m_remainingRange = newRange;
    emit remainingRangeChanged();
}

void VehicleController::setEnergyFlowState(int state)
{
    if (m_energyFlowState == state)
        return;

    m_energyFlowState = state;
    emit energyFlowStateChanged();
}

void VehicleController::setSpeed(int value)
{
    const int boundedValue = qBound(0, value, 240);
    if (m_speed == boundedValue)
        return;

    m_speed = boundedValue;
    emit speedChanged();
}

void VehicleController::setInstantPower(double value)
{
    const double roundedValue = roundedTenths(qBound(-80.0, value, 180.0));
    if (qAbs(m_instantPower - roundedValue) < 0.01)
        return;

    m_instantPower = roundedValue;
    emit instantPowerChanged();
}

void VehicleController::setAverageConsumption(double value)
{
    const double roundedValue = roundedTenths(qBound(8.0, value, 30.0));
    if (qAbs(m_averageConsumption - roundedValue) < 0.01)
        return;

    m_averageConsumption = roundedValue;
    emit averageConsumptionChanged();
}

void VehicleController::setBatteryTemperature(double value)
{
    const double roundedValue = roundedTenths(qBound(15.0, value, 65.0));
    if (qAbs(m_batteryTemperature - roundedValue) < 0.01)
        return;

    m_batteryTemperature = roundedValue;
    emit batteryTemperatureChanged();
}

void VehicleController::setMotorTemperature(double value)
{
    const double roundedValue = roundedTenths(qBound(20.0, value, 110.0));
    if (qAbs(m_motorTemperature - roundedValue) < 0.01)
        return;

    m_motorTemperature = roundedValue;
    emit motorTemperatureChanged();
}

void VehicleController::setBatteryLevel(int value)
{
    const int boundedValue = qBound(0, value, 100);
    if (m_batteryLevel == boundedValue)
        return;

    m_batteryLevel = boundedValue;
    emit batteryLevelChanged();
    updateRemainingRange();
}

void VehicleController::setTirePressures(double frontLeft,
                                         double frontRight,
                                         double rearLeft,
                                         double rearRight)
{
    const double newFrontLeft = roundedHundredths(qBound(1.5, frontLeft, 3.2));
    const double newFrontRight = roundedHundredths(qBound(1.5, frontRight, 3.2));
    const double newRearLeft = roundedHundredths(qBound(1.5, rearLeft, 3.2));
    const double newRearRight = roundedHundredths(qBound(1.5, rearRight, 3.2));

    if (qAbs(m_frontLeftPressure - newFrontLeft) < 0.01
        && qAbs(m_frontRightPressure - newFrontRight) < 0.01
        && qAbs(m_rearLeftPressure - newRearLeft) < 0.01
        && qAbs(m_rearRightPressure - newRearRight) < 0.01) {
        return;
    }

    m_frontLeftPressure = newFrontLeft;
    m_frontRightPressure = newFrontRight;
    m_rearLeftPressure = newRearLeft;
    m_rearRightPressure = newRearRight;
    emit tirePressureChanged();
}

double VehicleController::randomBetween(double minimum, double maximum)
{
    return minimum + (maximum - minimum)
                         * QRandomGenerator::global()->generateDouble();
}

double VehicleController::roundedTenths(double value)
{
    return std::round(value * 10.0) / 10.0;
}

double VehicleController::roundedHundredths(double value)
{
    return std::round(value * 100.0) / 100.0;
}
