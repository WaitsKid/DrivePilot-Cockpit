import QtQuick
import BYD163

Item {
    id: root

    width: 0
    height: 0
    visible: false

    property bool applyingSettings: false

    function applyAllSettings() {
        applyingSettings = true

        Ui.acLeftTemperature = AppSettings.acLeftTemperature
        Ui.acRightTemperature = AppSettings.acRightTemperature
        Ui.acFanLevel = AppSettings.acFanLevel

        Ui.settingsFunctionValue = AppSettings.settingsFunctionValue
        Ui.settingsLampHeight = AppSettings.settingsLampHeight
        Ui.settingsSteering = AppSettings.settingsSteering
        Ui.settingsParking = AppSettings.settingsParking
        Ui.settingsTrafficEnvironment = AppSettings.settingsTrafficEnvironment

        Ui.controlCenterWLANStatus = AppSettings.wlanEnabled
        Ui.controlCenterBluetoothStatus = AppSettings.bluetoothEnabled
        Ui.controlCenterPositionStatus = AppSettings.positionEnabled
        Ui.controlCenterMediaVolume = AppSettings.mediaVolume

        VehicleData.driveMode = AppSettings.driveMode
        MusicPlayer.volume = AppSettings.mediaVolume

        applyingSettings = false
    }

    Component.onCompleted: applyAllSettings()

    Connections {
        target: Ui

        function onAcLeftTemperatureChanged() {
            if (!root.applyingSettings
                    && AppSettings.acLeftTemperature !== Ui.acLeftTemperature)
                AppSettings.acLeftTemperature = Ui.acLeftTemperature
        }

        function onAcRightTemperatureChanged() {
            if (!root.applyingSettings
                    && AppSettings.acRightTemperature !== Ui.acRightTemperature)
                AppSettings.acRightTemperature = Ui.acRightTemperature
        }

        function onAcFanLevelChanged() {
            if (!root.applyingSettings && AppSettings.acFanLevel !== Ui.acFanLevel)
                AppSettings.acFanLevel = Ui.acFanLevel
        }

        function onSettingsFunctionValueChanged() {
            if (!root.applyingSettings
                    && AppSettings.settingsFunctionValue !== Ui.settingsFunctionValue)
                AppSettings.settingsFunctionValue = Ui.settingsFunctionValue
        }

        function onSettingsLampHeightChanged() {
            if (!root.applyingSettings
                    && AppSettings.settingsLampHeight !== Ui.settingsLampHeight)
                AppSettings.settingsLampHeight = Ui.settingsLampHeight
        }

        function onSettingsSteeringChanged() {
            if (!root.applyingSettings
                    && AppSettings.settingsSteering !== Ui.settingsSteering)
                AppSettings.settingsSteering = Ui.settingsSteering
        }

        function onSettingsParkingChanged() {
            if (!root.applyingSettings
                    && AppSettings.settingsParking !== Ui.settingsParking)
                AppSettings.settingsParking = Ui.settingsParking
        }

        function onSettingsTrafficEnvironmentChanged() {
            if (!root.applyingSettings
                    && AppSettings.settingsTrafficEnvironment !== Ui.settingsTrafficEnvironment)
                AppSettings.settingsTrafficEnvironment = Ui.settingsTrafficEnvironment
        }

        function onControlCenterWLANStatusChanged() {
            if (!root.applyingSettings
                    && AppSettings.wlanEnabled !== Ui.controlCenterWLANStatus)
                AppSettings.wlanEnabled = Ui.controlCenterWLANStatus
        }

        function onControlCenterBluetoothStatusChanged() {
            if (!root.applyingSettings
                    && AppSettings.bluetoothEnabled !== Ui.controlCenterBluetoothStatus)
                AppSettings.bluetoothEnabled = Ui.controlCenterBluetoothStatus
        }

        function onControlCenterPositionStatusChanged() {
            if (!root.applyingSettings
                    && AppSettings.positionEnabled !== Ui.controlCenterPositionStatus)
                AppSettings.positionEnabled = Ui.controlCenterPositionStatus
        }

        function onControlCenterMediaVolumeChanged() {
            if (MusicPlayer.volume !== Ui.controlCenterMediaVolume)
                MusicPlayer.volume = Ui.controlCenterMediaVolume

            if (!root.applyingSettings
                    && AppSettings.mediaVolume !== Ui.controlCenterMediaVolume)
                AppSettings.mediaVolume = Ui.controlCenterMediaVolume
        }
    }

    Connections {
        target: VehicleData

        function onDriveModeChanged() {
            if (!root.applyingSettings && AppSettings.driveMode !== VehicleData.driveMode)
                AppSettings.driveMode = VehicleData.driveMode
        }
    }

    Connections {
        target: MusicPlayer

        function onVolumeChanged() {
            if (Ui.controlCenterMediaVolume !== MusicPlayer.volume)
                Ui.controlCenterMediaVolume = MusicPlayer.volume

            if (!root.applyingSettings && AppSettings.mediaVolume !== MusicPlayer.volume)
                AppSettings.mediaVolume = MusicPlayer.volume
        }
    }

    Connections {
        target: AppSettings

        function onAcLeftTemperatureChanged() {
            if (Ui.acLeftTemperature !== AppSettings.acLeftTemperature)
                Ui.acLeftTemperature = AppSettings.acLeftTemperature
        }

        function onAcRightTemperatureChanged() {
            if (Ui.acRightTemperature !== AppSettings.acRightTemperature)
                Ui.acRightTemperature = AppSettings.acRightTemperature
        }

        function onAcFanLevelChanged() {
            if (Ui.acFanLevel !== AppSettings.acFanLevel)
                Ui.acFanLevel = AppSettings.acFanLevel
        }

        function onSettingsFunctionValueChanged() {
            if (Ui.settingsFunctionValue !== AppSettings.settingsFunctionValue)
                Ui.settingsFunctionValue = AppSettings.settingsFunctionValue
        }

        function onSettingsLampHeightChanged() {
            if (Ui.settingsLampHeight !== AppSettings.settingsLampHeight)
                Ui.settingsLampHeight = AppSettings.settingsLampHeight
        }

        function onSettingsSteeringChanged() {
            if (Ui.settingsSteering !== AppSettings.settingsSteering)
                Ui.settingsSteering = AppSettings.settingsSteering
        }

        function onSettingsParkingChanged() {
            if (Ui.settingsParking !== AppSettings.settingsParking)
                Ui.settingsParking = AppSettings.settingsParking
        }

        function onSettingsTrafficEnvironmentChanged() {
            if (Ui.settingsTrafficEnvironment !== AppSettings.settingsTrafficEnvironment)
                Ui.settingsTrafficEnvironment = AppSettings.settingsTrafficEnvironment
        }

        function onWlanEnabledChanged() {
            if (Ui.controlCenterWLANStatus !== AppSettings.wlanEnabled)
                Ui.controlCenterWLANStatus = AppSettings.wlanEnabled
        }

        function onBluetoothEnabledChanged() {
            if (Ui.controlCenterBluetoothStatus !== AppSettings.bluetoothEnabled)
                Ui.controlCenterBluetoothStatus = AppSettings.bluetoothEnabled
        }

        function onPositionEnabledChanged() {
            if (Ui.controlCenterPositionStatus !== AppSettings.positionEnabled)
                Ui.controlCenterPositionStatus = AppSettings.positionEnabled
        }

        function onMediaVolumeChanged() {
            if (Ui.controlCenterMediaVolume !== AppSettings.mediaVolume)
                Ui.controlCenterMediaVolume = AppSettings.mediaVolume
            if (MusicPlayer.volume !== AppSettings.mediaVolume)
                MusicPlayer.volume = AppSettings.mediaVolume
        }

        function onDriveModeChanged() {
            if (VehicleData.driveMode !== AppSettings.driveMode)
                VehicleData.driveMode = AppSettings.driveMode
        }
    }
}
