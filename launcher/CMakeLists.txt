# The bbpc launcher (Qt 6 Widgets, C++23): picks the game folder, edits the port's settings
# (bbport.ini) and the start-up options run.sh takes, starts and stops the game and shows its
# output. Built from the repository's root CMakeLists.txt, or on its own:
#   cmake -S launcher -B out-launcher -G Ninja && cmake --build out-launcher
cmake_minimum_required(VERSION 3.25)

if (NOT DEFINED BBPC_VERSION)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../version.json" BBPC_VERSION_JSON)
    string(JSON BBPC_VERSION GET "${BBPC_VERSION_JSON}" app_version)
endif()
project(bbpc_launcher VERSION ${BBPC_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Svg Network)

# The launcher's logic without widgets: settings, bbport.ini, the game's environment, the port's
# queries. The tests link it too.
add_library(bbpc_launcher_core STATIC
    debug.h
    core/appConfig.h
    core/appConfig.cpp
    core/capabilities.h
    core/capabilities.cpp
    core/choices.h
    core/choices.cpp
    core/desktopShortcut.h
    core/desktopShortcut.cpp
    core/fsr411Build.h
    core/fsr411Build.cpp
    core/gameEnvironment.h
    core/gameEnvironment.cpp
    core/graphicsSettings.h
    core/graphicsSettings.cpp
    core/hostEnvironment.h
    core/launcherSettings.h
    core/launcherSettings.cpp
    core/onlineAddresses.h
    core/onlineAddresses.cpp
    core/outputSizes.h
    core/outputSizes.cpp
    core/platformUtils.h
    core/portIni.h
    core/portIni.cpp
    core/portPaths.h
    core/portPaths.cpp
    core/profiles.h
    core/profiles.cpp
    core/trophies.h
    core/trophies.cpp
    core/updateCheck.h
    core/updateCheck.cpp
    core/upscalerAssets.h
    core/upscalerAssets.cpp
    process/groupProcess.h
    process/groupProcess.cpp
    process/portQueries.h
    process/portQueries.cpp
    process/processRunner.h
    process/processRunner.cpp
)
target_include_directories(bbpc_launcher_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/core")
target_link_libraries(bbpc_launcher_core PUBLIC Qt6::Core)
target_compile_options(bbpc_launcher_core PRIVATE -Wall -Wextra)

add_executable(bbpc
    main.cpp
    mainWindow.h
    mainWindow.cpp
    themeManager.h
    themeManager.cpp
    dialogs/aboutDialog.h
    dialogs/aboutDialog.cpp
    dialogs/updateAvailableDialog.h
    dialogs/updateAvailableDialog.cpp
    pages/launcherPage.h
    pages/advancedPage.h
    pages/advancedPage.cpp
    pages/controlsPage.h
    pages/controlsPage.cpp
    pages/displayPage.h
    pages/displayPage.cpp
    pages/gamePage.h
    pages/gamePage.cpp
    pages/graphicsPage.h
    pages/graphicsPage.cpp
    pages/logPage.h
    pages/logPage.cpp
    pages/modsPage.h
    pages/modsPage.cpp
    pages/onlinePage.h
    pages/onlinePage.cpp
    pages/trophiesPage.h
    pages/trophiesPage.cpp
    widgets/choiceCombo.h
    widgets/choiceCombo.cpp
    widgets/icons.h
    widgets/icons.cpp
    widgets/settingRows.h
    widgets/settingRows.cpp
    widgets/settingsGroup.h
    widgets/settingsGroup.cpp
    widgets/toastNotification.h
    widgets/toggleSwitch.h
    widgets/toggleSwitch.cpp
    widgets/uiHelpers.h
    resources.qrc
)
target_link_libraries(bbpc PRIVATE bbpc_launcher_core Qt6::Gui Qt6::Widgets Qt6::Svg Qt6::Network)
target_compile_options(bbpc PRIVATE -Wall -Wextra)

# Unit tests (tests/): with the root project's BBPC_BUILD_TESTS, or a Debug build of this one.
if (BBPC_BUILD_TESTS OR (PROJECT_IS_TOP_LEVEL AND CMAKE_BUILD_TYPE STREQUAL "Debug"))
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS bbpc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES io.github.wheat32.bbpc.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# AppStream data for software centers and AppImage managers (#287).
install(FILES io.github.wheat32.bbpc.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES assets/io.github.wheat32.bbpc.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
