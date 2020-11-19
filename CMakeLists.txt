cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(jtask CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(QT_MIN_VERSION "5.3.0")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets)

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_MODE 1)
else()
  set(DEBUG_MODE 0)
endif()

set(jtask_SRCS
    src/aboutdialog.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/trayicon.cpp
    src/taskdialog.cpp
    src/task.cpp
    src/datetimedialog.cpp
    src/configmanager.cpp
    src/optionaldatetimeedit.cpp
    src/settingsdialog.cpp
    src/recurringdialog.cpp
    src/recurringtasksmodel.cpp
    src/tasksview.cpp
    src/taskdescriptiondelegate.cpp
    src/qtutil.cpp
    src/tagsedit.cpp
    src/tasksmodel.cpp
    src/taskwatcher.cpp
    src/taskwarrior.cpp)
set(jtask_MOC_HEADERS src/configmanager.hpp)
set(jtask_RESOURCES res/jtask.qrc)
add_executable(jtask ${jtask_SRCS} ${jtask_RESOURCES})

target_link_libraries(jtask Qt5::Widgets Qt5::GuiPrivate)

set(JTASK_VERSION_MAJOR 1)
set(JTASK_VERSION_MINOR 0)
set(JTASK_VERSION_PATCH 0)
set_property(
  TARGET jtask
  PROPERTY VERSION
           ${JTASK_VERSION_MAJOR}.${JTASK_VERSION_MINOR}.${JTASK_VERSION_PATCH})
configure_file(src/config.hpp.in src/config.hpp)

include(GNUInstallDirs)
install(TARGETS jtask RUNTIME DESTINATION bin)
install(FILES res/icons/jtask.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/24x24/apps)
install(FILES res/jtask.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES res/icons/jtask.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
