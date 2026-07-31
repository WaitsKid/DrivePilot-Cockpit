#include "Interface.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QtGlobal>

namespace {
constexpr int kDefaultLeftTemperature = 26;
constexpr int kDefaultRightTemperature = 26;
constexpr int kDefaultFanLevel = 5;
constexpr int kDefaultAcMode = Interface::AC_MODE_NORMAL;
constexpr int kMaximumNavigationDepth = 32;
constexpr int kDefaultSettingsFunction = 0;
constexpr int kDefaultLampHeight = 7;
constexpr int kDefaultSteering = 1;
constexpr bool kDefaultParking = true;
constexpr int kDefaultTrafficEnvironment = 1;
constexpr int kDefaultBrakeAssistMode = 0;
constexpr bool kDefaultAmbientLightEnabled = true;
constexpr bool kDefaultAmbientDynamic = true;
constexpr bool kDefaultHudEnabled = true;
constexpr int kDefaultHudHeight = 5;
constexpr int kDefaultHudBrightness = 6;
constexpr int kDefaultHudRotation = 5;
constexpr int kDefaultWelcomeMode = 0;
constexpr bool kDefaultWelcomeUnlock = true;
constexpr bool kDefaultWelcomeSound = true;
constexpr int kDefaultMemoryProfile = 0;
constexpr bool kDefaultEasyEntry = true;
constexpr bool kDefaultMirrorReverse = true;
constexpr bool kDefaultMirrorAutoFold = true;
constexpr bool kDefaultAcSync = true;
constexpr bool kDefaultAcAutoDefog = true;
constexpr bool kDefaultAcPurify = true;
constexpr int kDefaultAcFragrance = 1;
constexpr bool kDefaultDoorAutoLock = true;
constexpr bool kDefaultDoorUnlockOnPark = true;
constexpr bool kDefaultChildLock = false;
constexpr int kDefaultTailgateHeight = 6;
constexpr bool kDefaultFatigueReminder = true;
constexpr bool kDefaultSpeedLimitReminder = true;
constexpr bool kDefaultDepartureReminder = true;
constexpr int kDefaultReminderVolume = 6;
constexpr bool kDefaultWlan = true;
constexpr bool kDefaultBluetooth = true;
constexpr bool kDefaultPosition = true;
constexpr int kDefaultMediaVolume = 7;
}

Interface::Interface(QObject *parent)
    : QObject(parent)
{
    m_loadingPreferences = true;
    loadPreferences();
    m_loadingPreferences = false;

    m_preferencesSaveTimer.setSingleShot(true);
    m_preferencesSaveTimer.setInterval(300);
    connect(&m_preferencesSaveTimer,
            &QTimer::timeout,
            this,
            &Interface::savePreferences);

    m_updateTimer.setInterval(1000);
    m_updateTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_updateTimer,
            &QTimer::timeout,
            this,
            &Interface::refreshDateTime);
    m_updateTimer.start();
    QTimer::singleShot(0, this, &Interface::refreshDateTime);
}

Interface::~Interface()
{
    if (m_preferencesSaveTimer.isActive())
        savePreferences();
}

int Interface::getPAGE_MAIN() const { return PAGE_MAIN; }
int Interface::getPAGE_HOME() const { return PAGE_HOME; }
int Interface::getPAGE_AC() const { return PAGE_AC; }
int Interface::getPAGE_APP() const { return PAGE_APP; }
int Interface::getPAGE_SETTINGS() const { return PAGE_SETTINGS; }
int Interface::getPAGE_VEHICLE() const { return PAGE_VEHICLE; }
int Interface::getPAGE_MUSIC() const { return PAGE_MUSIC; }
int Interface::getPAGE_CONTROL() const { return PAGE_CONTROL; }
int Interface::getPAGE_WEATHER() const { return PAGE_WEATHER; }
int Interface::getPAGE_ASSISTANT() const { return PAGE_ASSISTANT; }
int Interface::getPAGE_CONTACTS() const { return PAGE_CONTACTS; }
int Interface::getPAGE_VIDEO() const { return PAGE_VIDEO; }
int Interface::getPAGE_CALCULATOR() const { return PAGE_CALCULATOR; }
int Interface::getPAGE_VECTOR_STUDIO() const { return PAGE_VECTOR_STUDIO; }
int Interface::getPAGE_MAP() const { return PAGE_MAP; }

int Interface::getAC_MODE_NORMAL() const { return AC_MODE_NORMAL; }
int Interface::getAC_MODE_DRY() const { return AC_MODE_DRY; }
int Interface::getAC_MODE_BOOST() const { return AC_MODE_BOOST; }
int Interface::getAC_MODE_AUTO() const { return AC_MODE_AUTO; }

int Interface::getPageIndex() const { return m_pageIndex; }
void Interface::setPageIndex(int newPageIndex) { navigateTo(newPageIndex); }

int Interface::getPreviousPageIndex() const { return m_previousPageIndex; }
void Interface::setPreviousPageIndex(int newPreviousPageIndex)
{
    if (!isValidPage(newPreviousPageIndex) || m_previousPageIndex == newPreviousPageIndex)
        return;

    m_previousPageIndex = newPreviousPageIndex;
    emit previousPageIndexChanged();
}

bool Interface::getCanGoBack() const
{
    return !m_navigationHistory.isEmpty() || m_pageIndex != PAGE_HOME;
}

int Interface::getScreenRotation() const { return m_screenRotation; }
void Interface::setScreenRotation(int newScreenRotation)
{
    const int normalizedRotation = (qAbs(newScreenRotation) % 180 == 90) ? 90 : 0;
    if (m_screenRotation == normalizedRotation)
        return;

    m_screenRotation = normalizedRotation;
    emit screenRotationChanged();
}

int Interface::getAcLeftTemperature() const { return m_acLeftTemperature; }
void Interface::setAcLeftTemperature(int newAcLeftTemperature)
{
    const int value = qBound(16, newAcLeftTemperature, 32);
    if (m_acLeftTemperature == value)
        return;
    m_acLeftTemperature = value;
    emit acLeftTemperatureChanged();
    schedulePreferencesSave();
}

int Interface::getAcRightTemperature() const { return m_acRightTemperature; }
void Interface::setAcRightTemperature(int newAcRightTemperature)
{
    const int value = qBound(16, newAcRightTemperature, 32);
    if (m_acRightTemperature == value)
        return;
    m_acRightTemperature = value;
    emit acRightTemperatureChanged();
    schedulePreferencesSave();
}

int Interface::getAcFanLevel() const { return m_acFanLevel; }
void Interface::setAcFanLevel(int newAcFanLevel)
{
    const int value = qBound(0, newAcFanLevel, 10);
    if (m_acFanLevel == value)
        return;
    m_acFanLevel = value;
    emit acFanLevelChanged();
    schedulePreferencesSave();
}

int Interface::getAcMode() const { return m_acMode; }
void Interface::setAcMode(int newAcMode)
{
    const int value = qBound(AC_MODE_NORMAL, newAcMode, AC_MODE_AUTO);
    if (m_acMode == value)
        return;

    m_acMode = value;
    emit acModeChanged();

    if (!m_loadingPreferences) {
        if (m_acMode == AC_MODE_NORMAL)
            setAcFanLevel(5);
        else if (m_acMode == AC_MODE_BOOST)
            setAcFanLevel(10);
    }

    schedulePreferencesSave();
}

QString Interface::getAcModeText() const
{
    switch (m_acMode) {
    case AC_MODE_DRY: return QStringLiteral("除湿");
    case AC_MODE_BOOST: return QStringLiteral("加强");
    case AC_MODE_AUTO: return QStringLiteral("自动");
    case AC_MODE_NORMAL:
    default: return QStringLiteral("正常");
    }
}

int Interface::getSettingsFunctionValue() const { return m_settingsFunctionValue; }
void Interface::setSettingsFunctionValue(int newSettingsFunctionValue)
{
    const int value = qBound(0, newSettingsFunctionValue, 7);
    if (m_settingsFunctionValue == value)
        return;
    m_settingsFunctionValue = value;
    emit settingsFunctionValueChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsLampHeight() const { return m_settingsLampHeight; }
void Interface::setSettingsLampHeight(int newSettingsLampHeight)
{
    const int value = qBound(0, newSettingsLampHeight, 10);
    if (m_settingsLampHeight == value)
        return;
    m_settingsLampHeight = value;
    emit settingsLampHeightChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsSteering() const { return m_settingsSteering; }
void Interface::setSettingsSteering(int newSettingsSteering)
{
    const int value = qBound(0, newSettingsSteering, 2);
    if (m_settingsSteering == value)
        return;
    m_settingsSteering = value;
    emit settingsSteeringChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsParking() const { return m_settingsParking; }
void Interface::setSettingsParking(bool newSettingsParking)
{
    if (m_settingsParking == newSettingsParking)
        return;
    m_settingsParking = newSettingsParking;
    emit settingsParkingChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsTrafficEnvironment() const { return m_settingsTrafficEnvironment; }
void Interface::setSettingsTrafficEnvironment(int newSettingsTrafficEnvironment)
{
    const int value = qBound(0, newSettingsTrafficEnvironment, 2);
    if (m_settingsTrafficEnvironment == value)
        return;
    m_settingsTrafficEnvironment = value;
    emit settingsTrafficEnvironmentChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsBrakeAssistMode() const { return m_settingsBrakeAssistMode; }
void Interface::setSettingsBrakeAssistMode(int newSettingsBrakeAssistMode)
{
    const int value = qBound(0, newSettingsBrakeAssistMode, 2);
    if (m_settingsBrakeAssistMode == value)
        return;
    m_settingsBrakeAssistMode = value;
    emit settingsBrakeAssistModeChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsAmbientLightEnabled() const { return m_settingsAmbientLightEnabled; }
void Interface::setSettingsAmbientLightEnabled(bool newSettingsAmbientLightEnabled)
{
    if (m_settingsAmbientLightEnabled == newSettingsAmbientLightEnabled)
        return;
    m_settingsAmbientLightEnabled = newSettingsAmbientLightEnabled;
    emit settingsAmbientLightEnabledChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsAmbientDynamic() const { return m_settingsAmbientDynamic; }
void Interface::setSettingsAmbientDynamic(bool newSettingsAmbientDynamic)
{
    if (m_settingsAmbientDynamic == newSettingsAmbientDynamic)
        return;
    m_settingsAmbientDynamic = newSettingsAmbientDynamic;
    emit settingsAmbientDynamicChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsHudEnabled() const { return m_settingsHudEnabled; }
void Interface::setSettingsHudEnabled(bool newSettingsHudEnabled)
{
    if (m_settingsHudEnabled == newSettingsHudEnabled)
        return;
    m_settingsHudEnabled = newSettingsHudEnabled;
    emit settingsHudEnabledChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsHudHeight() const { return m_settingsHudHeight; }
void Interface::setSettingsHudHeight(int newSettingsHudHeight)
{
    const int value = qBound(0, newSettingsHudHeight, 10);
    if (m_settingsHudHeight == value)
        return;
    m_settingsHudHeight = value;
    emit settingsHudHeightChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsHudBrightness() const { return m_settingsHudBrightness; }
void Interface::setSettingsHudBrightness(int newSettingsHudBrightness)
{
    const int value = qBound(0, newSettingsHudBrightness, 10);
    if (m_settingsHudBrightness == value)
        return;
    m_settingsHudBrightness = value;
    emit settingsHudBrightnessChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsHudRotation() const { return m_settingsHudRotation; }
void Interface::setSettingsHudRotation(int newSettingsHudRotation)
{
    const int value = qBound(0, newSettingsHudRotation, 10);
    if (m_settingsHudRotation == value)
        return;
    m_settingsHudRotation = value;
    emit settingsHudRotationChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsWelcomeMode() const { return m_settingsWelcomeMode; }
void Interface::setSettingsWelcomeMode(int newSettingsWelcomeMode)
{
    const int value = qBound(0, newSettingsWelcomeMode, 2);
    if (m_settingsWelcomeMode == value)
        return;
    m_settingsWelcomeMode = value;
    emit settingsWelcomeModeChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsWelcomeUnlock() const { return m_settingsWelcomeUnlock; }
void Interface::setSettingsWelcomeUnlock(bool newSettingsWelcomeUnlock)
{
    if (m_settingsWelcomeUnlock == newSettingsWelcomeUnlock)
        return;
    m_settingsWelcomeUnlock = newSettingsWelcomeUnlock;
    emit settingsWelcomeUnlockChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsWelcomeSound() const { return m_settingsWelcomeSound; }
void Interface::setSettingsWelcomeSound(bool newSettingsWelcomeSound)
{
    if (m_settingsWelcomeSound == newSettingsWelcomeSound)
        return;
    m_settingsWelcomeSound = newSettingsWelcomeSound;
    emit settingsWelcomeSoundChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsMemoryProfile() const { return m_settingsMemoryProfile; }
void Interface::setSettingsMemoryProfile(int newSettingsMemoryProfile)
{
    const int value = qBound(0, newSettingsMemoryProfile, 2);
    if (m_settingsMemoryProfile == value)
        return;
    m_settingsMemoryProfile = value;
    emit settingsMemoryProfileChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsEasyEntry() const { return m_settingsEasyEntry; }
void Interface::setSettingsEasyEntry(bool newSettingsEasyEntry)
{
    if (m_settingsEasyEntry == newSettingsEasyEntry)
        return;
    m_settingsEasyEntry = newSettingsEasyEntry;
    emit settingsEasyEntryChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsMirrorReverse() const { return m_settingsMirrorReverse; }
void Interface::setSettingsMirrorReverse(bool newSettingsMirrorReverse)
{
    if (m_settingsMirrorReverse == newSettingsMirrorReverse)
        return;
    m_settingsMirrorReverse = newSettingsMirrorReverse;
    emit settingsMirrorReverseChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsMirrorAutoFold() const { return m_settingsMirrorAutoFold; }
void Interface::setSettingsMirrorAutoFold(bool newSettingsMirrorAutoFold)
{
    if (m_settingsMirrorAutoFold == newSettingsMirrorAutoFold)
        return;
    m_settingsMirrorAutoFold = newSettingsMirrorAutoFold;
    emit settingsMirrorAutoFoldChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsAcSync() const { return m_settingsAcSync; }
void Interface::setSettingsAcSync(bool newSettingsAcSync)
{
    if (m_settingsAcSync == newSettingsAcSync)
        return;
    m_settingsAcSync = newSettingsAcSync;
    emit settingsAcSyncChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsAcAutoDefog() const { return m_settingsAcAutoDefog; }
void Interface::setSettingsAcAutoDefog(bool newSettingsAcAutoDefog)
{
    if (m_settingsAcAutoDefog == newSettingsAcAutoDefog)
        return;
    m_settingsAcAutoDefog = newSettingsAcAutoDefog;
    emit settingsAcAutoDefogChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsAcPurify() const { return m_settingsAcPurify; }
void Interface::setSettingsAcPurify(bool newSettingsAcPurify)
{
    if (m_settingsAcPurify == newSettingsAcPurify)
        return;
    m_settingsAcPurify = newSettingsAcPurify;
    emit settingsAcPurifyChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsAcFragrance() const { return m_settingsAcFragrance; }
void Interface::setSettingsAcFragrance(int newSettingsAcFragrance)
{
    const int value = qBound(0, newSettingsAcFragrance, 2);
    if (m_settingsAcFragrance == value)
        return;
    m_settingsAcFragrance = value;
    emit settingsAcFragranceChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsDoorAutoLock() const { return m_settingsDoorAutoLock; }
void Interface::setSettingsDoorAutoLock(bool newSettingsDoorAutoLock)
{
    if (m_settingsDoorAutoLock == newSettingsDoorAutoLock)
        return;
    m_settingsDoorAutoLock = newSettingsDoorAutoLock;
    emit settingsDoorAutoLockChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsDoorUnlockOnPark() const { return m_settingsDoorUnlockOnPark; }
void Interface::setSettingsDoorUnlockOnPark(bool newSettingsDoorUnlockOnPark)
{
    if (m_settingsDoorUnlockOnPark == newSettingsDoorUnlockOnPark)
        return;
    m_settingsDoorUnlockOnPark = newSettingsDoorUnlockOnPark;
    emit settingsDoorUnlockOnParkChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsChildLock() const { return m_settingsChildLock; }
void Interface::setSettingsChildLock(bool newSettingsChildLock)
{
    if (m_settingsChildLock == newSettingsChildLock)
        return;
    m_settingsChildLock = newSettingsChildLock;
    emit settingsChildLockChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsTailgateHeight() const { return m_settingsTailgateHeight; }
void Interface::setSettingsTailgateHeight(int newSettingsTailgateHeight)
{
    const int value = qBound(0, newSettingsTailgateHeight, 10);
    if (m_settingsTailgateHeight == value)
        return;
    m_settingsTailgateHeight = value;
    emit settingsTailgateHeightChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsFatigueReminder() const { return m_settingsFatigueReminder; }
void Interface::setSettingsFatigueReminder(bool newSettingsFatigueReminder)
{
    if (m_settingsFatigueReminder == newSettingsFatigueReminder)
        return;
    m_settingsFatigueReminder = newSettingsFatigueReminder;
    emit settingsFatigueReminderChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsSpeedLimitReminder() const { return m_settingsSpeedLimitReminder; }
void Interface::setSettingsSpeedLimitReminder(bool newSettingsSpeedLimitReminder)
{
    if (m_settingsSpeedLimitReminder == newSettingsSpeedLimitReminder)
        return;
    m_settingsSpeedLimitReminder = newSettingsSpeedLimitReminder;
    emit settingsSpeedLimitReminderChanged();
    schedulePreferencesSave();
}

bool Interface::getSettingsDepartureReminder() const { return m_settingsDepartureReminder; }
void Interface::setSettingsDepartureReminder(bool newSettingsDepartureReminder)
{
    if (m_settingsDepartureReminder == newSettingsDepartureReminder)
        return;
    m_settingsDepartureReminder = newSettingsDepartureReminder;
    emit settingsDepartureReminderChanged();
    schedulePreferencesSave();
}

int Interface::getSettingsReminderVolume() const { return m_settingsReminderVolume; }
void Interface::setSettingsReminderVolume(int newSettingsReminderVolume)
{
    const int value = qBound(0, newSettingsReminderVolume, 10);
    if (m_settingsReminderVolume == value)
        return;
    m_settingsReminderVolume = value;
    emit settingsReminderVolumeChanged();
    schedulePreferencesSave();
}

bool Interface::getControlCenterWLANStatus() const { return m_controlCenterWLANStatus; }
void Interface::setControlCenterWLANStatus(bool newControlCenterWLANStatus)
{
    if (m_controlCenterWLANStatus == newControlCenterWLANStatus)
        return;
    m_controlCenterWLANStatus = newControlCenterWLANStatus;
    emit controlCenterWLANStatusChanged();
    schedulePreferencesSave();
}

bool Interface::getControlCenterBluetoothStatus() const { return m_controlCenterBluetoothStatus; }
void Interface::setControlCenterBluetoothStatus(bool newControlCenterBluetoothStatus)
{
    if (m_controlCenterBluetoothStatus == newControlCenterBluetoothStatus)
        return;
    m_controlCenterBluetoothStatus = newControlCenterBluetoothStatus;
    emit controlCenterBluetoothStatusChanged();
    schedulePreferencesSave();
}

bool Interface::getControlCenterPositionStatus() const { return m_controlCenterPositionStatus; }
void Interface::setControlCenterPositionStatus(bool newControlCenterPositionStatus)
{
    if (m_controlCenterPositionStatus == newControlCenterPositionStatus)
        return;
    m_controlCenterPositionStatus = newControlCenterPositionStatus;
    emit controlCenterPositionStatusChanged();
    schedulePreferencesSave();
}

int Interface::getControlCenterMediaVolume() const { return m_controlCenterMediaVolume; }
void Interface::setControlCenterMediaVolume(int newControlCenterMediaVolume)
{
    const int value = qBound(0, newControlCenterMediaVolume, 10);
    if (m_controlCenterMediaVolume == value)
        return;
    m_controlCenterMediaVolume = value;
    emit controlCenterMediaVolumeChanged();
    schedulePreferencesSave();
}

void Interface::navigateTo(int page)
{
    if (!isValidPage(page) || m_pageIndex == page)
        return;

    if (m_navigationHistory.isEmpty() || m_navigationHistory.constLast() != m_pageIndex)
        m_navigationHistory.append(m_pageIndex);

    while (m_navigationHistory.size() > kMaximumNavigationDepth)
        m_navigationHistory.removeFirst();

    m_pageIndex = page;
    updatePreviousPageFromHistory();
    emit pageIndexChanged();
    emit navigationHistoryChanged();
}

void Interface::goBack()
{
    int targetPage = PAGE_HOME;
    bool foundTarget = false;

    while (!m_navigationHistory.isEmpty()) {
        const int candidate = m_navigationHistory.takeLast();
        if (isValidPage(candidate) && candidate != m_pageIndex) {
            targetPage = candidate;
            foundTarget = true;
            break;
        }
    }

    if (!foundTarget && m_pageIndex == PAGE_HOME) {
        updatePreviousPageFromHistory();
        emit navigationHistoryChanged();
        return;
    }

    m_pageIndex = targetPage;
    updatePreviousPageFromHistory();
    emit pageIndexChanged();
    emit navigationHistoryChanged();
}

void Interface::goHome()
{
    const bool pageChanged = m_pageIndex != PAGE_HOME;
    const bool historyChanged = !m_navigationHistory.isEmpty();

    m_navigationHistory.clear();
    m_pageIndex = PAGE_HOME;
    updatePreviousPageFromHistory();

    if (pageChanged)
        emit pageIndexChanged();
    if (pageChanged || historyChanged)
        emit navigationHistoryChanged();
}

void Interface::toggleScreenRotation()
{
    setScreenRotation(m_screenRotation == 0 ? 90 : 0);
    emit toastRequested(m_screenRotation == 90
                            ? QStringLiteral("已切换为竖屏显示")
                            : QStringLiteral("已切换为横屏显示"));
}

void Interface::selectAcMode(int mode)
{
    const int normalizedMode = qBound(AC_MODE_NORMAL, mode, AC_MODE_AUTO);
    setAcMode(normalizedMode);

    QString detail;
    if (normalizedMode == AC_MODE_NORMAL) {
        setAcFanLevel(5);
        detail = QStringLiteral("，风量已调整为 5 档");
    } else if (normalizedMode == AC_MODE_BOOST) {
        setAcFanLevel(10);
        detail = QStringLiteral("，风量已调整为 10 档");
    }

    emit toastRequested(QStringLiteral("空调已切换为%1模式%2")
                            .arg(getAcModeText(), detail));
}

void Interface::quitApplication()
{
    m_preferencesSaveTimer.stop();
    savePreferences();
    QCoreApplication::quit();
}

void Interface::showToast(const QString &message)
{
    const QString text = message.trimmed();
    if (!text.isEmpty())
        emit toastRequested(text);
}

void Interface::resetUserPreferences()
{
    setAcLeftTemperature(kDefaultLeftTemperature);
    setAcRightTemperature(kDefaultRightTemperature);
    setAcFanLevel(kDefaultFanLevel);
    setAcMode(kDefaultAcMode);

    setSettingsFunctionValue(kDefaultSettingsFunction);
    setSettingsLampHeight(kDefaultLampHeight);
    setSettingsSteering(kDefaultSteering);
    setSettingsParking(kDefaultParking);
    setSettingsTrafficEnvironment(kDefaultTrafficEnvironment);
    setSettingsBrakeAssistMode(kDefaultBrakeAssistMode);
    setSettingsAmbientLightEnabled(kDefaultAmbientLightEnabled);
    setSettingsAmbientDynamic(kDefaultAmbientDynamic);
    setSettingsHudEnabled(kDefaultHudEnabled);
    setSettingsHudHeight(kDefaultHudHeight);
    setSettingsHudBrightness(kDefaultHudBrightness);
    setSettingsHudRotation(kDefaultHudRotation);
    setSettingsWelcomeMode(kDefaultWelcomeMode);
    setSettingsWelcomeUnlock(kDefaultWelcomeUnlock);
    setSettingsWelcomeSound(kDefaultWelcomeSound);
    setSettingsMemoryProfile(kDefaultMemoryProfile);
    setSettingsEasyEntry(kDefaultEasyEntry);
    setSettingsMirrorReverse(kDefaultMirrorReverse);
    setSettingsMirrorAutoFold(kDefaultMirrorAutoFold);
    setSettingsAcSync(kDefaultAcSync);
    setSettingsAcAutoDefog(kDefaultAcAutoDefog);
    setSettingsAcPurify(kDefaultAcPurify);
    setSettingsAcFragrance(kDefaultAcFragrance);
    setSettingsDoorAutoLock(kDefaultDoorAutoLock);
    setSettingsDoorUnlockOnPark(kDefaultDoorUnlockOnPark);
    setSettingsChildLock(kDefaultChildLock);
    setSettingsTailgateHeight(kDefaultTailgateHeight);
    setSettingsFatigueReminder(kDefaultFatigueReminder);
    setSettingsSpeedLimitReminder(kDefaultSpeedLimitReminder);
    setSettingsDepartureReminder(kDefaultDepartureReminder);
    setSettingsReminderVolume(kDefaultReminderVolume);

    setControlCenterWLANStatus(kDefaultWlan);
    setControlCenterBluetoothStatus(kDefaultBluetooth);
    setControlCenterPositionStatus(kDefaultPosition);
    setControlCenterMediaVolume(kDefaultMediaVolume);

    m_preferencesSaveTimer.stop();
    savePreferences();
    emit userPreferencesReset();
    emit toastRequested(QStringLiteral("已恢复默认设置"));
}

void Interface::refreshDateTime()
{
    const QDateTime current = QDateTime::currentDateTime();
    const QDate dateValue = current.date();

    static const QStringList weekNames = {
        QString(),
        QStringLiteral("星期一"),
        QStringLiteral("星期二"),
        QStringLiteral("星期三"),
        QStringLiteral("星期四"),
        QStringLiteral("星期五"),
        QStringLiteral("星期六"),
        QStringLiteral("星期日")
    };

    const QString date = QStringLiteral("%1 %2")
                             .arg(dateValue.toString(QStringLiteral("M月d日")),
                                  weekNames.value(dateValue.dayOfWeek()));
    const QString time = current.time().toString(QStringLiteral("HH:mm"));

    emit updateDateTime(date, time);
}

void Interface::loadPreferences()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("preferences"));

    m_acLeftTemperature = qBound(16, settings.value(QStringLiteral("acLeftTemperature"), kDefaultLeftTemperature).toInt(), 32);
    m_acRightTemperature = qBound(16, settings.value(QStringLiteral("acRightTemperature"), kDefaultRightTemperature).toInt(), 32);
    m_acFanLevel = qBound(0, settings.value(QStringLiteral("acFanLevel"), kDefaultFanLevel).toInt(), 10);
    m_acMode = qBound(AC_MODE_NORMAL, settings.value(QStringLiteral("acMode"), kDefaultAcMode).toInt(), AC_MODE_AUTO);

    m_settingsFunctionValue = qBound(0, settings.value(QStringLiteral("settingsFunctionValue"), kDefaultSettingsFunction).toInt(), 7);
    m_settingsLampHeight = qBound(0, settings.value(QStringLiteral("settingsLampHeight"), kDefaultLampHeight).toInt(), 10);
    m_settingsSteering = qBound(0, settings.value(QStringLiteral("settingsSteering"), kDefaultSteering).toInt(), 2);
    m_settingsParking = settings.value(QStringLiteral("settingsParking"), kDefaultParking).toBool();
    m_settingsTrafficEnvironment = qBound(0, settings.value(QStringLiteral("settingsTrafficEnvironment"), kDefaultTrafficEnvironment).toInt(), 2);
    m_settingsBrakeAssistMode = qBound(0, settings.value(QStringLiteral("settingsBrakeAssistMode"), kDefaultBrakeAssistMode).toInt(), 2);
    m_settingsAmbientLightEnabled = settings.value(QStringLiteral("settingsAmbientLightEnabled"), kDefaultAmbientLightEnabled).toBool();
    m_settingsAmbientDynamic = settings.value(QStringLiteral("settingsAmbientDynamic"), kDefaultAmbientDynamic).toBool();
    m_settingsHudEnabled = settings.value(QStringLiteral("settingsHudEnabled"), kDefaultHudEnabled).toBool();
    m_settingsHudHeight = qBound(0, settings.value(QStringLiteral("settingsHudHeight"), kDefaultHudHeight).toInt(), 10);
    m_settingsHudBrightness = qBound(0, settings.value(QStringLiteral("settingsHudBrightness"), kDefaultHudBrightness).toInt(), 10);
    m_settingsHudRotation = qBound(0, settings.value(QStringLiteral("settingsHudRotation"), kDefaultHudRotation).toInt(), 10);
    m_settingsWelcomeMode = qBound(0, settings.value(QStringLiteral("settingsWelcomeMode"), kDefaultWelcomeMode).toInt(), 2);
    m_settingsWelcomeUnlock = settings.value(QStringLiteral("settingsWelcomeUnlock"), kDefaultWelcomeUnlock).toBool();
    m_settingsWelcomeSound = settings.value(QStringLiteral("settingsWelcomeSound"), kDefaultWelcomeSound).toBool();
    m_settingsMemoryProfile = qBound(0, settings.value(QStringLiteral("settingsMemoryProfile"), kDefaultMemoryProfile).toInt(), 2);
    m_settingsEasyEntry = settings.value(QStringLiteral("settingsEasyEntry"), kDefaultEasyEntry).toBool();
    m_settingsMirrorReverse = settings.value(QStringLiteral("settingsMirrorReverse"), kDefaultMirrorReverse).toBool();
    m_settingsMirrorAutoFold = settings.value(QStringLiteral("settingsMirrorAutoFold"), kDefaultMirrorAutoFold).toBool();
    m_settingsAcSync = settings.value(QStringLiteral("settingsAcSync"), kDefaultAcSync).toBool();
    m_settingsAcAutoDefog = settings.value(QStringLiteral("settingsAcAutoDefog"), kDefaultAcAutoDefog).toBool();
    m_settingsAcPurify = settings.value(QStringLiteral("settingsAcPurify"), kDefaultAcPurify).toBool();
    m_settingsAcFragrance = qBound(0, settings.value(QStringLiteral("settingsAcFragrance"), kDefaultAcFragrance).toInt(), 2);
    m_settingsDoorAutoLock = settings.value(QStringLiteral("settingsDoorAutoLock"), kDefaultDoorAutoLock).toBool();
    m_settingsDoorUnlockOnPark = settings.value(QStringLiteral("settingsDoorUnlockOnPark"), kDefaultDoorUnlockOnPark).toBool();
    m_settingsChildLock = settings.value(QStringLiteral("settingsChildLock"), kDefaultChildLock).toBool();
    m_settingsTailgateHeight = qBound(0, settings.value(QStringLiteral("settingsTailgateHeight"), kDefaultTailgateHeight).toInt(), 10);
    m_settingsFatigueReminder = settings.value(QStringLiteral("settingsFatigueReminder"), kDefaultFatigueReminder).toBool();
    m_settingsSpeedLimitReminder = settings.value(QStringLiteral("settingsSpeedLimitReminder"), kDefaultSpeedLimitReminder).toBool();
    m_settingsDepartureReminder = settings.value(QStringLiteral("settingsDepartureReminder"), kDefaultDepartureReminder).toBool();
    m_settingsReminderVolume = qBound(0, settings.value(QStringLiteral("settingsReminderVolume"), kDefaultReminderVolume).toInt(), 10);

    m_controlCenterWLANStatus = settings.value(QStringLiteral("wlanEnabled"), kDefaultWlan).toBool();
    m_controlCenterBluetoothStatus = settings.value(QStringLiteral("bluetoothEnabled"), kDefaultBluetooth).toBool();
    m_controlCenterPositionStatus = settings.value(QStringLiteral("positionEnabled"), kDefaultPosition).toBool();
    m_controlCenterMediaVolume = qBound(0, settings.value(QStringLiteral("mediaVolume"), kDefaultMediaVolume).toInt(), 10);

    settings.endGroup();
}

void Interface::schedulePreferencesSave()
{
    if (!m_loadingPreferences)
        m_preferencesSaveTimer.start();
}

void Interface::savePreferences()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("preferences"));

    settings.setValue(QStringLiteral("acLeftTemperature"), m_acLeftTemperature);
    settings.setValue(QStringLiteral("acRightTemperature"), m_acRightTemperature);
    settings.setValue(QStringLiteral("acFanLevel"), m_acFanLevel);
    settings.setValue(QStringLiteral("acMode"), m_acMode);

    settings.setValue(QStringLiteral("settingsFunctionValue"), m_settingsFunctionValue);
    settings.setValue(QStringLiteral("settingsLampHeight"), m_settingsLampHeight);
    settings.setValue(QStringLiteral("settingsSteering"), m_settingsSteering);
    settings.setValue(QStringLiteral("settingsParking"), m_settingsParking);
    settings.setValue(QStringLiteral("settingsTrafficEnvironment"), m_settingsTrafficEnvironment);
    settings.setValue(QStringLiteral("settingsBrakeAssistMode"), m_settingsBrakeAssistMode);
    settings.setValue(QStringLiteral("settingsAmbientLightEnabled"), m_settingsAmbientLightEnabled);
    settings.setValue(QStringLiteral("settingsAmbientDynamic"), m_settingsAmbientDynamic);
    settings.setValue(QStringLiteral("settingsHudEnabled"), m_settingsHudEnabled);
    settings.setValue(QStringLiteral("settingsHudHeight"), m_settingsHudHeight);
    settings.setValue(QStringLiteral("settingsHudBrightness"), m_settingsHudBrightness);
    settings.setValue(QStringLiteral("settingsHudRotation"), m_settingsHudRotation);
    settings.setValue(QStringLiteral("settingsWelcomeMode"), m_settingsWelcomeMode);
    settings.setValue(QStringLiteral("settingsWelcomeUnlock"), m_settingsWelcomeUnlock);
    settings.setValue(QStringLiteral("settingsWelcomeSound"), m_settingsWelcomeSound);
    settings.setValue(QStringLiteral("settingsMemoryProfile"), m_settingsMemoryProfile);
    settings.setValue(QStringLiteral("settingsEasyEntry"), m_settingsEasyEntry);
    settings.setValue(QStringLiteral("settingsMirrorReverse"), m_settingsMirrorReverse);
    settings.setValue(QStringLiteral("settingsMirrorAutoFold"), m_settingsMirrorAutoFold);
    settings.setValue(QStringLiteral("settingsAcSync"), m_settingsAcSync);
    settings.setValue(QStringLiteral("settingsAcAutoDefog"), m_settingsAcAutoDefog);
    settings.setValue(QStringLiteral("settingsAcPurify"), m_settingsAcPurify);
    settings.setValue(QStringLiteral("settingsAcFragrance"), m_settingsAcFragrance);
    settings.setValue(QStringLiteral("settingsDoorAutoLock"), m_settingsDoorAutoLock);
    settings.setValue(QStringLiteral("settingsDoorUnlockOnPark"), m_settingsDoorUnlockOnPark);
    settings.setValue(QStringLiteral("settingsChildLock"), m_settingsChildLock);
    settings.setValue(QStringLiteral("settingsTailgateHeight"), m_settingsTailgateHeight);
    settings.setValue(QStringLiteral("settingsFatigueReminder"), m_settingsFatigueReminder);
    settings.setValue(QStringLiteral("settingsSpeedLimitReminder"), m_settingsSpeedLimitReminder);
    settings.setValue(QStringLiteral("settingsDepartureReminder"), m_settingsDepartureReminder);
    settings.setValue(QStringLiteral("settingsReminderVolume"), m_settingsReminderVolume);

    settings.setValue(QStringLiteral("wlanEnabled"), m_controlCenterWLANStatus);
    settings.setValue(QStringLiteral("bluetoothEnabled"), m_controlCenterBluetoothStatus);
    settings.setValue(QStringLiteral("positionEnabled"), m_controlCenterPositionStatus);
    settings.setValue(QStringLiteral("mediaVolume"), m_controlCenterMediaVolume);

    settings.endGroup();
    settings.sync();
}

void Interface::updatePreviousPageFromHistory()
{
    const int previousPage = m_navigationHistory.isEmpty() ? PAGE_HOME : m_navigationHistory.constLast();
    if (m_previousPageIndex == previousPage)
        return;

    m_previousPageIndex = previousPage;
    emit previousPageIndexChanged();
}

bool Interface::isValidPage(int page) const
{
    return page >= PAGE_MAIN && page <= PAGE_MAP;
}
