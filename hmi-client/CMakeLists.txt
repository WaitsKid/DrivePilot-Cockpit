cmake_minimum_required(VERSION 3.16)

project(DrivePilotCockpit VERSION 1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia Network WebSockets Sql Concurrent WebEngineQuick WebChannel)
find_package(Qt6 QUIET COMPONENTS Positioning)
find_package(Qt6 QUIET COMPONENTS TextToSpeech)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appDrivePilot
        main.cpp
)

qt_add_qml_module(appDrivePilot
        URI DrivePilot
        QML_FILES
        Main.qml
        Components/SwipeArea.qml
        Components/ToastMessage.qml
        Components/IconButton.qml
        Components/IconSwitch.qml
        Components/FunctionBar3.qml
        Components/FunctionBar2.qml
        Components/ColorSlider.qml
        Components/ACFunctionBar.qml
        Components/ACFan.qml
        Components/QuickWind.qml
        Components/QuickSlider.qml
        Components/ACBar.qml
        Components/StatusBar.qml Components/PageChrome.qml
        Components/SettingsModeBar.qml
        Components/SettingsList.qml
        Components/SettingsFunctionBar.qml
        Components/SettingsBridge.qml
        Components/QuickTemperatureList.qml
        Components/Navigation.qml
        Components/ShutdownDialog.qml
        Components/ACModePanel.qml
        Components/CircularGauge.qml
        Components/MetricCard.qml
        Components/TirePressureCard.qml
        Components/DriveModeSelector.qml
        Components/EnergyFlow.qml
        Components/MusicCover.qml
        Components/PlaybackButton.qml
        Components/AudioVisualizer.qml
        Components/AppTile.qml
        Components/WeatherMetricCard.qml
        Components/WeatherIcon.qml
        Components/HourlyWeatherDelegate.qml
        Components/DailyWeatherDelegate.qml
        Components/CitySuggestionDelegate.qml
        Components/MapPlaceSuggestionDelegate.qml
        Components/ChatMessageDelegate.qml
        Components/ContactCard.qml
        Components/CallHistoryDelegate.qml
        Components/DialPadButton.qml
        Components/VideoListDelegate.qml
        Components/CalculatorKey.qml
        Components/StudioButton.qml
        Components/StudioSlider.qml
        Components/StudioSwitch.qml
        Components/StudioScrollBar.qml
        Components/DmsStatusIndicator.qml
        Components/DmsAlertToast.qml
        Components/AiAgentToolBridge.qml
        HMI/Home.qml
        HMI/AC.qml
        HMI/App.qml
        HMI/ControlCenter.qml
        HMI/Settings.qml
        HMI/VehicleHealth.qml
        HMI/MusicPage.qml
        HMI/WeatherPage.qml
        HMI/VoiceAssistantPage.qml
        HMI/ContactPage.qml
        HMI/VideoCenterPage.qml
        HMI/ScientificCalculatorPage.qml
        HMI/MapPage.qml
        HMI/VectorStudioPage.qml
        SOURCES
        Interface/Interface.h
        Interface/Interface.cpp
        Interface/VehicleController.h
        Interface/VehicleController.cpp
        Interface/MediaLibraryStorage.h
        Interface/MediaLibraryStorage.cpp
        Interface/MusicController.h
        Interface/MusicController.cpp
        Interface/AppLauncherModel.h
        Interface/AppLauncherModel.cpp
        Interface/WeatherListModel.h
        Interface/WeatherListModel.cpp
        Interface/CitySuggestionModel.h
        Interface/CitySuggestionModel.cpp
        Interface/WeatherController.h
        Interface/WeatherController.cpp
        Interface/AssistantMessageModel.h
        Interface/AssistantMessageModel.cpp
        Interface/VoiceAssistantController.h
        Interface/VoiceAssistantController.cpp
        Interface/ContactListModel.h
        Interface/ContactListModel.cpp
        Interface/CallHistoryModel.h
        Interface/CallHistoryModel.cpp
        Interface/ContactController.h
        Interface/ContactController.cpp
        Interface/VideoController.h
        Interface/VideoController.cpp
        Interface/ScientificCalculator.h
        Interface/ScientificCalculator.cpp
        Interface/MapSearchResultModel.h
        Interface/MapSearchResultModel.cpp
        Interface/NavigationStepModel.h
        Interface/NavigationStepModel.cpp
        Interface/MapController.h
        Interface/MapController.cpp
        Interface/VectorStudioController.h
        Interface/VectorStudioController.cpp
        Interface/DmsController.h
        Interface/DmsController.cpp
        RESOURCES
        resource.qrc
        Web/navigation.html
        Web/navigation.css
        Web/navigation.js
)

qt_add_resources(appDrivePilot dms_status_icons
        PREFIX "/"
        FILES
        Images/Dms/energetic.png
        Images/Dms/normal.png
        Images/Dms/slight_fatigue.png
        Images/Dms/severe_fatigue.png
)
# C++ 接口头文件目录
target_include_directories(appDrivePilot
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Interface
)
# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appDrivePilot PROPERTIES
        #    MACOSX_BUNDLE_GUI_IDENTIFIER com.waitskid.drivepilot
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
)

if (WIN32)
    set_target_properties(appDrivePilot PROPERTIES WIN32_EXECUTABLE TRUE)
    target_compile_definitions(appDrivePilot PRIVATE DRIVEPILOT_PLATFORM_WINDOWS=1)
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(appDrivePilot PRIVATE DRIVEPILOT_PLATFORM_LINUX=1)
endif()

target_compile_features(appDrivePilot PRIVATE cxx_std_17)

target_link_libraries(appDrivePilot
        PRIVATE Qt6::Quick
        Qt6::Multimedia
        Qt6::Network
        Qt6::WebSockets
        Qt6::Sql
        Qt6::Concurrent
        Qt6::WebEngineQuick
        Qt6::WebChannel
)

if (TARGET Qt6::Positioning)
    target_link_libraries(appDrivePilot PRIVATE Qt6::Positioning)
    target_compile_definitions(appDrivePilot PRIVATE DRIVEPILOT_HAS_QT_POSITIONING=1)
endif()

if (TARGET Qt6::TextToSpeech)
    target_link_libraries(appDrivePilot PRIVATE Qt6::TextToSpeech)
    target_compile_definitions(appDrivePilot PRIVATE DRIVEPILOT_HAS_QT_TEXT_TO_SPEECH=1)
endif()

# 讯飞凭据使用外部 config.json，不编译进 qrc 或可执行文件。
# config.json 已加入 .gitignore；GitHub 仅保留 config.example.json。
set(DRIVEPILOT_RUNTIME_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/config.json")
if (EXISTS "${DRIVEPILOT_RUNTIME_CONFIG}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${DRIVEPILOT_RUNTIME_CONFIG}")
    configure_file("${DRIVEPILOT_RUNTIME_CONFIG}"
            "${CMAKE_CURRENT_BINARY_DIR}/config.json"
            COPYONLY)
    add_custom_command(TARGET appDrivePilot POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DRIVEPILOT_RUNTIME_CONFIG}"
            "$<TARGET_FILE_DIR:appDrivePilot>/config.json"
            VERBATIM)
endif()

# 视频采用外部运行时资源，而不是打进 qrc。
# 这样可以避免大型媒体膨胀可执行文件，也能让 QMediaPlayer 直接读取普通文件 URL。
set(DRIVEPILOT_VIDEO_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Videos")
if (EXISTS "${DRIVEPILOT_VIDEO_ASSET_DIR}")
    add_custom_target(copy_video_assets ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DRIVEPILOT_VIDEO_ASSET_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/Videos"
            COMMENT "Copying video assets")
    add_dependencies(appDrivePilot copy_video_assets)

    add_custom_command(TARGET appDrivePilot POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DRIVEPILOT_VIDEO_ASSET_DIR}"
            "$<TARGET_FILE_DIR:appDrivePilot>/Videos"
            VERBATIM)
endif()

install(TARGETS appDrivePilot
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
