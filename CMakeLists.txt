cmake_minimum_required(VERSION 3.16)
project(kaeru VERSION 0.2.0 LANGUAGES CXX DESCRIPTION "Kaeru - dependable keep awake utility")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

set(KAERU_APP_ID "io.github.kaeru.Kaeru")
add_compile_definitions(KAERU_APP_ID="${KAERU_APP_ID}")
add_compile_definitions(KAERU_VERSION="${PROJECT_VERSION}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus Sql Test)

# ---- core library (no DBus/SQL/Widgets) ----
add_library(kaeru_core STATIC
  src/core/Types.cpp
  src/core/Clock.cpp
  src/core/Duration.cpp
  src/core/InhibitionRegistry.cpp
  src/core/InhibitionCoordinator.cpp
  src/core/InhibitionBackend.cpp
  src/core/FakeInhibitionBackend.cpp
  src/core/ManualAwakeSession.cpp
  src/core/TimerEngine.cpp
  src/core/FocusController.cpp
  src/core/CapabilityRegistry.cpp
  src/core/AppState.cpp
)
target_include_directories(kaeru_core PUBLIC src)
target_link_libraries(kaeru_core PUBLIC Qt6::Core)

# ---- storage library ----
add_library(kaeru_storage STATIC
  src/storage/Database.cpp
  src/storage/Migrations.cpp
  src/storage/ProjectRepository.cpp
  src/storage/SessionRepository.cpp
  src/storage/CsvExport.cpp
)
target_link_libraries(kaeru_storage PUBLIC Qt6::Core Qt6::Sql kaeru_core)

# ---- platform library (DBus adapters) ----
add_library(kaeru_platform STATIC
  src/platform/PortalInhibitBackend.cpp
  src/platform/LogindInhibitBackend.cpp
  src/platform/LogindPowerMonitor.cpp
  src/platform/LockMonitor.cpp
  src/platform/Notifications.cpp
  src/platform/AutostartBackend.cpp
  src/platform/SingleInstance.cpp
  src/platform/ThemeWatcher.cpp
)
target_link_libraries(kaeru_platform PUBLIC Qt6::Core Qt6::DBus kaeru_core)

# ---- app library (shared by GUI + CLI) ----
add_library(kaeru_app STATIC
  src/app/Application.cpp
  src/app/CommandRouter.cpp
  src/app/ControlService.cpp
  src/app/CliParser.cpp
  src/app/Paths.cpp
  src/app/RecoveryManager.cpp
  src/app/Settings.cpp
  src/app/WorkSessionManager.cpp
)
target_link_libraries(kaeru_app PUBLIC Qt6::Core Qt6::DBus kaeru_core kaeru_platform kaeru_storage)

# ---- UI library ----
add_library(kaeru_ui STATIC
  src/ui/MainWindow.cpp
  src/ui/Theme.cpp
  src/ui/Icons.cpp
  src/ui/TrayController.cpp
  src/ui/widgets/StatusBadge.cpp
  src/ui/pages/OverviewPage.cpp
  src/ui/pages/FocusPage.cpp
  src/ui/pages/WorkLogPage.cpp
  src/ui/pages/ReportsPage.cpp
  src/ui/pages/SettingsPage.cpp
  src/ui/pages/DiagnosticsPage.cpp
)
target_link_libraries(kaeru_ui PUBLIC Qt6::Widgets Qt6::Gui kaeru_app)

# ---- main executable ----
# Embedded UI icons use the supplied PNG raster variants: the original SVGs
# are installed for desktop integration, but Qt 6.2's SVG renderer mis-paints
# the focused frog (black face). PNGs guarantee identical artwork everywhere.
set(KAERU_FROG_PNGS)
foreach(px 16 24 32 48 64 128 256)
  foreach(state awake focused sleep)
    list(APPEND KAERU_FROG_PNGS resources/icons/frog/png/${state}-${px}.png)
  endforeach()
endforeach()

qt_add_resources(kaeru_ui RESOURCES_PREFIX "/" PREFIX "/" BASE resources
  FILES
  ${KAERU_FROG_PNGS}
)

add_executable(kaeru src/app/main.cpp)
target_link_libraries(kaeru PRIVATE kaeru_ui Qt6::Widgets)

# ---- unit tests ----
enable_testing()
foreach(t IN ITEMS clocks registry coordinator timerengine focus worklog storage csv cliparser notify workmanager reports traystate manualawake icons)
  add_executable(test_${t} tests/unit/test_${t}.cpp)
  target_link_libraries(test_${t} PRIVATE kaeru_app kaeru_ui Qt6::Test)
  add_test(NAME test_${t} COMMAND test_${t})
  set_tests_properties(test_${t} PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;KAERU_TEST_MODE=1")
endforeach()

install(TARGETS kaeru RUNTIME DESTINATION bin)
install(FILES resources/io.github.kaeru.Kaeru.desktop DESTINATION share/applications RENAME io.github.kaeru.Kaeru.desktop)
install(FILES resources/io.github.kaeru.Kaeru.metainfo.xml DESTINATION share/metainfo RENAME io.github.kaeru.Kaeru.metainfo.xml)
install(FILES README.md DESTINATION share/doc/kaeru)
install(FILES kaeru.1 DESTINATION share/man/man1)
install(FILES LICENSE DESTINATION share/doc/kaeru)

# Official frog icon identity (ADR-008): scalable SVGs + raster fallbacks
# for panels that lack SVG support.
install(FILES resources/icons/frog/awake.svg
  DESTINATION share/icons/hicolor/scalable/apps
  RENAME io.github.kaeru.Kaeru.svg)
install(FILES resources/icons/frog/focused.svg
  DESTINATION share/icons/hicolor/scalable/apps
  RENAME io.github.kaeru.Kaeru.focused.svg)
install(FILES resources/icons/frog/sleep.svg
  DESTINATION share/icons/hicolor/scalable/apps
  RENAME io.github.kaeru.Kaeru.sleep.svg)
foreach(px 16 24 32 48 64 128 256)
  foreach(state awake focused sleep)
    install(FILES resources/icons/frog/png/${state}-${px}.png
      DESTINATION share/icons/hicolor/${px}x${px}/apps
      RENAME io.github.kaeru.Kaeru-${state}.png)
  endforeach()
endforeach()
