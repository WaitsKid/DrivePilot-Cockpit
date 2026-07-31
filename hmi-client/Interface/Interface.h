#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include <qqmlintegration.h>

class Interface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Ui)

    Q_PROPERTY(int PAGE_MAIN READ getPAGE_MAIN CONSTANT)
    Q_PROPERTY(int PAGE_HOME READ getPAGE_HOME CONSTANT)
    Q_PROPERTY(int PAGE_AC READ getPAGE_AC CONSTANT)
    Q_PROPERTY(int PAGE_APP READ getPAGE_APP CONSTANT)
    Q_PROPERTY(int PAGE_SETTINGS READ getPAGE_SETTINGS CONSTANT)
    Q_PROPERTY(int PAGE_VEHICLE READ getPAGE_VEHICLE CONSTANT)
    Q_PROPERTY(int PAGE_MUSIC READ getPAGE_MUSIC CONSTANT)
    Q_PROPERTY(int PAGE_CONTROL READ getPAGE_CONTROL CONSTANT)
    Q_PROPERTY(int PAGE_WEATHER READ getPAGE_WEATHER CONSTANT)
    Q_PROPERTY(int PAGE_ASSISTANT READ getPAGE_ASSISTANT CONSTANT)
    Q_PROPERTY(int PAGE_CONTACTS READ getPAGE_CONTACTS CONSTANT)
    Q_PROPERTY(int PAGE_VIDEO READ getPAGE_VIDEO CONSTANT)
    Q_PROPERTY(int PAGE_CALCULATOR READ getPAGE_CALCULATOR CONSTANT)
    Q_PROPERTY(int PAGE_VECTOR_STUDIO READ getPAGE_VECTOR_STUDIO CONSTANT)
    Q_PROPERTY(int PAGE_MAP READ getPAGE_MAP CONSTANT)

    Q_PROPERTY(int AC_MODE_NORMAL READ getAC_MODE_NORMAL CONSTANT)
    Q_PROPERTY(int AC_MODE_DRY READ getAC_MODE_DRY CONSTANT)
    Q_PROPERTY(int AC_MODE_BOOST READ getAC_MODE_BOOST CONSTANT)
    Q_PROPERTY(int AC_MODE_AUTO READ getAC_MODE_AUTO CONSTANT)

    Q_PROPERTY(int pageIndex READ getPageIndex WRITE setPageIndex NOTIFY pageIndexChanged)
    Q_PROPERTY(int previousPageIndex READ getPreviousPageIndex WRITE setPreviousPageIndex NOTIFY previousPageIndexChanged)
    Q_PROPERTY(bool canGoBack READ getCanGoBack NOTIFY navigationHistoryChanged)
    Q_PROPERTY(int screenRotation READ getScreenRotation WRITE setScreenRotation NOTIFY screenRotationChanged)

    Q_PROPERTY(int acLeftTemperature READ getAcLeftTemperature WRITE setAcLeftTemperature NOTIFY acLeftTemperatureChanged)
    Q_PROPERTY(int acRightTemperature READ getAcRightTemperature WRITE setAcRightTemperature NOTIFY acRightTemperatureChanged)
    Q_PROPERTY(int acFanLevel READ getAcFanLevel WRITE setAcFanLevel NOTIFY acFanLevelChanged)
    Q_PROPERTY(int acMode READ getAcMode WRITE setAcMode NOTIFY acModeChanged)
    Q_PROPERTY(QString acModeText READ getAcModeText NOTIFY acModeChanged)

    Q_PROPERTY(int settingsFunctionValue READ getSettingsFunctionValue WRITE setSettingsFunctionValue NOTIFY settingsFunctionValueChanged)
    Q_PROPERTY(int settingsLampHeight READ getSettingsLampHeight WRITE setSettingsLampHeight NOTIFY settingsLampHeightChanged)
    Q_PROPERTY(int settingsSteering READ getSettingsSteering WRITE setSettingsSteering NOTIFY settingsSteeringChanged)
    Q_PROPERTY(bool settingsParking READ getSettingsParking WRITE setSettingsParking NOTIFY settingsParkingChanged)
    Q_PROPERTY(int settingsTrafficEnvironment READ getSettingsTrafficEnvironment WRITE setSettingsTrafficEnvironment NOTIFY settingsTrafficEnvironmentChanged)
    Q_PROPERTY(int settingsBrakeAssistMode READ getSettingsBrakeAssistMode WRITE setSettingsBrakeAssistMode NOTIFY settingsBrakeAssistModeChanged)
    Q_PROPERTY(bool settingsAmbientLightEnabled READ getSettingsAmbientLightEnabled WRITE setSettingsAmbientLightEnabled NOTIFY settingsAmbientLightEnabledChanged)
    Q_PROPERTY(bool settingsAmbientDynamic READ getSettingsAmbientDynamic WRITE setSettingsAmbientDynamic NOTIFY settingsAmbientDynamicChanged)

    Q_PROPERTY(bool settingsHudEnabled READ getSettingsHudEnabled WRITE setSettingsHudEnabled NOTIFY settingsHudEnabledChanged)
    Q_PROPERTY(int settingsHudHeight READ getSettingsHudHeight WRITE setSettingsHudHeight NOTIFY settingsHudHeightChanged)
    Q_PROPERTY(int settingsHudBrightness READ getSettingsHudBrightness WRITE setSettingsHudBrightness NOTIFY settingsHudBrightnessChanged)
    Q_PROPERTY(int settingsHudRotation READ getSettingsHudRotation WRITE setSettingsHudRotation NOTIFY settingsHudRotationChanged)

    Q_PROPERTY(int settingsWelcomeMode READ getSettingsWelcomeMode WRITE setSettingsWelcomeMode NOTIFY settingsWelcomeModeChanged)
    Q_PROPERTY(bool settingsWelcomeUnlock READ getSettingsWelcomeUnlock WRITE setSettingsWelcomeUnlock NOTIFY settingsWelcomeUnlockChanged)
    Q_PROPERTY(bool settingsWelcomeSound READ getSettingsWelcomeSound WRITE setSettingsWelcomeSound NOTIFY settingsWelcomeSoundChanged)

    Q_PROPERTY(int settingsMemoryProfile READ getSettingsMemoryProfile WRITE setSettingsMemoryProfile NOTIFY settingsMemoryProfileChanged)
    Q_PROPERTY(bool settingsEasyEntry READ getSettingsEasyEntry WRITE setSettingsEasyEntry NOTIFY settingsEasyEntryChanged)
    Q_PROPERTY(bool settingsMirrorReverse READ getSettingsMirrorReverse WRITE setSettingsMirrorReverse NOTIFY settingsMirrorReverseChanged)
    Q_PROPERTY(bool settingsMirrorAutoFold READ getSettingsMirrorAutoFold WRITE setSettingsMirrorAutoFold NOTIFY settingsMirrorAutoFoldChanged)

    Q_PROPERTY(bool settingsAcSync READ getSettingsAcSync WRITE setSettingsAcSync NOTIFY settingsAcSyncChanged)
    Q_PROPERTY(bool settingsAcAutoDefog READ getSettingsAcAutoDefog WRITE setSettingsAcAutoDefog NOTIFY settingsAcAutoDefogChanged)
    Q_PROPERTY(bool settingsAcPurify READ getSettingsAcPurify WRITE setSettingsAcPurify NOTIFY settingsAcPurifyChanged)
    Q_PROPERTY(int settingsAcFragrance READ getSettingsAcFragrance WRITE setSettingsAcFragrance NOTIFY settingsAcFragranceChanged)

    Q_PROPERTY(bool settingsDoorAutoLock READ getSettingsDoorAutoLock WRITE setSettingsDoorAutoLock NOTIFY settingsDoorAutoLockChanged)
    Q_PROPERTY(bool settingsDoorUnlockOnPark READ getSettingsDoorUnlockOnPark WRITE setSettingsDoorUnlockOnPark NOTIFY settingsDoorUnlockOnParkChanged)
    Q_PROPERTY(bool settingsChildLock READ getSettingsChildLock WRITE setSettingsChildLock NOTIFY settingsChildLockChanged)
    Q_PROPERTY(int settingsTailgateHeight READ getSettingsTailgateHeight WRITE setSettingsTailgateHeight NOTIFY settingsTailgateHeightChanged)

    Q_PROPERTY(bool settingsFatigueReminder READ getSettingsFatigueReminder WRITE setSettingsFatigueReminder NOTIFY settingsFatigueReminderChanged)
    Q_PROPERTY(bool settingsSpeedLimitReminder READ getSettingsSpeedLimitReminder WRITE setSettingsSpeedLimitReminder NOTIFY settingsSpeedLimitReminderChanged)
    Q_PROPERTY(bool settingsDepartureReminder READ getSettingsDepartureReminder WRITE setSettingsDepartureReminder NOTIFY settingsDepartureReminderChanged)
    Q_PROPERTY(int settingsReminderVolume READ getSettingsReminderVolume WRITE setSettingsReminderVolume NOTIFY settingsReminderVolumeChanged)

    Q_PROPERTY(bool controlCenterWLANStatus READ getControlCenterWLANStatus WRITE setControlCenterWLANStatus NOTIFY controlCenterWLANStatusChanged)
    Q_PROPERTY(bool controlCenterBluetoothStatus READ getControlCenterBluetoothStatus WRITE setControlCenterBluetoothStatus NOTIFY controlCenterBluetoothStatusChanged)
    Q_PROPERTY(bool controlCenterPositionStatus READ getControlCenterPositionStatus WRITE setControlCenterPositionStatus NOTIFY controlCenterPositionStatusChanged)
    Q_PROPERTY(int controlCenterMediaVolume READ getControlCenterMediaVolume WRITE setControlCenterMediaVolume NOTIFY controlCenterMediaVolumeChanged)

public:
    explicit Interface(QObject *parent = nullptr);
    ~Interface() override;

    static constexpr int PAGE_MAIN = 0;
    static constexpr int PAGE_HOME = 1;
    static constexpr int PAGE_AC = 2;
    static constexpr int PAGE_APP = 3;
    static constexpr int PAGE_SETTINGS = 4;
    static constexpr int PAGE_VEHICLE = 5;
    static constexpr int PAGE_MUSIC = 6;
    static constexpr int PAGE_CONTROL = 7;
    static constexpr int PAGE_WEATHER = 8;
    static constexpr int PAGE_ASSISTANT = 9;
    static constexpr int PAGE_CONTACTS = 10;
    static constexpr int PAGE_VIDEO = 11;
    static constexpr int PAGE_CALCULATOR = 12;
    static constexpr int PAGE_VECTOR_STUDIO = 13;
    static constexpr int PAGE_MAP = 14;

    static constexpr int AC_MODE_NORMAL = 0;
    static constexpr int AC_MODE_DRY = 1;
    static constexpr int AC_MODE_BOOST = 2;
    static constexpr int AC_MODE_AUTO = 3;

    int getPAGE_MAIN() const;
    int getPAGE_HOME() const;
    int getPAGE_AC() const;
    int getPAGE_APP() const;
    int getPAGE_SETTINGS() const;
    int getPAGE_VEHICLE() const;
    int getPAGE_MUSIC() const;
    int getPAGE_CONTROL() const;
    int getPAGE_WEATHER() const;
    int getPAGE_ASSISTANT() const;
    int getPAGE_CONTACTS() const;
    int getPAGE_VIDEO() const;
    int getPAGE_CALCULATOR() const;
    int getPAGE_VECTOR_STUDIO() const;
    int getPAGE_MAP() const;

    int getAC_MODE_NORMAL() const;
    int getAC_MODE_DRY() const;
    int getAC_MODE_BOOST() const;
    int getAC_MODE_AUTO() const;

    int getPageIndex() const;
    void setPageIndex(int newPageIndex);

    int getPreviousPageIndex() const;
    void setPreviousPageIndex(int newPreviousPageIndex);
    bool getCanGoBack() const;

    int getScreenRotation() const;
    void setScreenRotation(int newScreenRotation);

    int getAcLeftTemperature() const;
    void setAcLeftTemperature(int newAcLeftTemperature);
    int getAcRightTemperature() const;
    void setAcRightTemperature(int newAcRightTemperature);
    int getAcFanLevel() const;
    void setAcFanLevel(int newAcFanLevel);
    int getAcMode() const;
    void setAcMode(int newAcMode);
    QString getAcModeText() const;

    int getSettingsFunctionValue() const;
    void setSettingsFunctionValue(int newSettingsFunctionValue);
    int getSettingsLampHeight() const;
    void setSettingsLampHeight(int newSettingsLampHeight);
    int getSettingsSteering() const;
    void setSettingsSteering(int newSettingsSteering);
    bool getSettingsParking() const;
    void setSettingsParking(bool newSettingsParking);
    int getSettingsTrafficEnvironment() const;
    void setSettingsTrafficEnvironment(int newSettingsTrafficEnvironment);
    int getSettingsBrakeAssistMode() const;
    void setSettingsBrakeAssistMode(int newSettingsBrakeAssistMode);
    bool getSettingsAmbientLightEnabled() const;
    void setSettingsAmbientLightEnabled(bool newSettingsAmbientLightEnabled);
    bool getSettingsAmbientDynamic() const;
    void setSettingsAmbientDynamic(bool newSettingsAmbientDynamic);

    bool getSettingsHudEnabled() const;
    void setSettingsHudEnabled(bool newSettingsHudEnabled);
    int getSettingsHudHeight() const;
    void setSettingsHudHeight(int newSettingsHudHeight);
    int getSettingsHudBrightness() const;
    void setSettingsHudBrightness(int newSettingsHudBrightness);
    int getSettingsHudRotation() const;
    void setSettingsHudRotation(int newSettingsHudRotation);

    int getSettingsWelcomeMode() const;
    void setSettingsWelcomeMode(int newSettingsWelcomeMode);
    bool getSettingsWelcomeUnlock() const;
    void setSettingsWelcomeUnlock(bool newSettingsWelcomeUnlock);
    bool getSettingsWelcomeSound() const;
    void setSettingsWelcomeSound(bool newSettingsWelcomeSound);

    int getSettingsMemoryProfile() const;
    void setSettingsMemoryProfile(int newSettingsMemoryProfile);
    bool getSettingsEasyEntry() const;
    void setSettingsEasyEntry(bool newSettingsEasyEntry);
    bool getSettingsMirrorReverse() const;
    void setSettingsMirrorReverse(bool newSettingsMirrorReverse);
    bool getSettingsMirrorAutoFold() const;
    void setSettingsMirrorAutoFold(bool newSettingsMirrorAutoFold);

    bool getSettingsAcSync() const;
    void setSettingsAcSync(bool newSettingsAcSync);
    bool getSettingsAcAutoDefog() const;
    void setSettingsAcAutoDefog(bool newSettingsAcAutoDefog);
    bool getSettingsAcPurify() const;
    void setSettingsAcPurify(bool newSettingsAcPurify);
    int getSettingsAcFragrance() const;
    void setSettingsAcFragrance(int newSettingsAcFragrance);

    bool getSettingsDoorAutoLock() const;
    void setSettingsDoorAutoLock(bool newSettingsDoorAutoLock);
    bool getSettingsDoorUnlockOnPark() const;
    void setSettingsDoorUnlockOnPark(bool newSettingsDoorUnlockOnPark);
    bool getSettingsChildLock() const;
    void setSettingsChildLock(bool newSettingsChildLock);
    int getSettingsTailgateHeight() const;
    void setSettingsTailgateHeight(int newSettingsTailgateHeight);

    bool getSettingsFatigueReminder() const;
    void setSettingsFatigueReminder(bool newSettingsFatigueReminder);
    bool getSettingsSpeedLimitReminder() const;
    void setSettingsSpeedLimitReminder(bool newSettingsSpeedLimitReminder);
    bool getSettingsDepartureReminder() const;
    void setSettingsDepartureReminder(bool newSettingsDepartureReminder);
    int getSettingsReminderVolume() const;
    void setSettingsReminderVolume(int newSettingsReminderVolume);

    bool getControlCenterWLANStatus() const;
    void setControlCenterWLANStatus(bool newControlCenterWLANStatus);
    bool getControlCenterBluetoothStatus() const;
    void setControlCenterBluetoothStatus(bool newControlCenterBluetoothStatus);
    bool getControlCenterPositionStatus() const;
    void setControlCenterPositionStatus(bool newControlCenterPositionStatus);
    int getControlCenterMediaVolume() const;
    void setControlCenterMediaVolume(int newControlCenterMediaVolume);

    Q_INVOKABLE void navigateTo(int page);
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void goHome();
    Q_INVOKABLE void toggleScreenRotation();
    Q_INVOKABLE void selectAcMode(int mode);
    Q_INVOKABLE void quitApplication();
    Q_INVOKABLE void showToast(const QString &message);
    Q_INVOKABLE void resetUserPreferences();

signals:
    void pageIndexChanged();
    void previousPageIndexChanged();
    void navigationHistoryChanged();
    void screenRotationChanged();
    void updateDateTime(const QString &date, const QString &time);
    void toastRequested(const QString &message);
    void userPreferencesReset();

    void acLeftTemperatureChanged();
    void acRightTemperatureChanged();
    void acFanLevelChanged();
    void acModeChanged();

    void settingsFunctionValueChanged();
    void settingsLampHeightChanged();
    void settingsSteeringChanged();
    void settingsParkingChanged();
    void settingsTrafficEnvironmentChanged();
    void settingsBrakeAssistModeChanged();
    void settingsAmbientLightEnabledChanged();
    void settingsAmbientDynamicChanged();
    void settingsHudEnabledChanged();
    void settingsHudHeightChanged();
    void settingsHudBrightnessChanged();
    void settingsHudRotationChanged();
    void settingsWelcomeModeChanged();
    void settingsWelcomeUnlockChanged();
    void settingsWelcomeSoundChanged();
    void settingsMemoryProfileChanged();
    void settingsEasyEntryChanged();
    void settingsMirrorReverseChanged();
    void settingsMirrorAutoFoldChanged();
    void settingsAcSyncChanged();
    void settingsAcAutoDefogChanged();
    void settingsAcPurifyChanged();
    void settingsAcFragranceChanged();
    void settingsDoorAutoLockChanged();
    void settingsDoorUnlockOnParkChanged();
    void settingsChildLockChanged();
    void settingsTailgateHeightChanged();
    void settingsFatigueReminderChanged();
    void settingsSpeedLimitReminderChanged();
    void settingsDepartureReminderChanged();
    void settingsReminderVolumeChanged();

    void controlCenterWLANStatusChanged();
    void controlCenterBluetoothStatusChanged();
    void controlCenterPositionStatusChanged();
    void controlCenterMediaVolumeChanged();

private slots:
    void refreshDateTime();
    void savePreferences();

private:
    bool isValidPage(int page) const;
    void loadPreferences();
    void schedulePreferencesSave();
    void updatePreviousPageFromHistory();

    QTimer m_updateTimer;
    QTimer m_preferencesSaveTimer;
    bool m_loadingPreferences = false;

    int m_pageIndex = PAGE_HOME;
    int m_previousPageIndex = PAGE_HOME;
    QVector<int> m_navigationHistory;
    int m_screenRotation = 0;

    int m_acLeftTemperature = 26;
    int m_acRightTemperature = 26;
    int m_acFanLevel = 5;
    int m_acMode = AC_MODE_NORMAL;

    int m_settingsFunctionValue = 0;
    int m_settingsSteering = 1;
    int m_settingsTrafficEnvironment = 1;
    bool m_settingsParking = true;
    int m_settingsLampHeight = 7;
    int m_settingsBrakeAssistMode = 0;
    bool m_settingsAmbientLightEnabled = true;
    bool m_settingsAmbientDynamic = true;

    bool m_settingsHudEnabled = true;
    int m_settingsHudHeight = 5;
    int m_settingsHudBrightness = 6;
    int m_settingsHudRotation = 5;

    int m_settingsWelcomeMode = 0;
    bool m_settingsWelcomeUnlock = true;
    bool m_settingsWelcomeSound = true;

    int m_settingsMemoryProfile = 0;
    bool m_settingsEasyEntry = true;
    bool m_settingsMirrorReverse = true;
    bool m_settingsMirrorAutoFold = true;

    bool m_settingsAcSync = true;
    bool m_settingsAcAutoDefog = true;
    bool m_settingsAcPurify = true;
    int m_settingsAcFragrance = 1;

    bool m_settingsDoorAutoLock = true;
    bool m_settingsDoorUnlockOnPark = true;
    bool m_settingsChildLock = false;
    int m_settingsTailgateHeight = 6;

    bool m_settingsFatigueReminder = true;
    bool m_settingsSpeedLimitReminder = true;
    bool m_settingsDepartureReminder = true;
    int m_settingsReminderVolume = 6;

    bool m_controlCenterWLANStatus = true;
    bool m_controlCenterBluetoothStatus = true;
    bool m_controlCenterPositionStatus = true;
    int m_controlCenterMediaVolume = 7;
};

#endif
