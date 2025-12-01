cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(qtask CXX)

set(QTASK_VERSION_MAJOR 1)
set(QTASK_VERSION_MINOR 1)
set(QTASK_VERSION_PATCH 6)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


option(BUILD_TESTS "Build tests" OFF)

#Detect best qt version in the system.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
if (${QT_VERSION} VERSION_LESS 5.15.0)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)
else()
  #Not sure, if should search/link GuiPrivate Svg with old versions.
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets GuiPrivate Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)
endif()

message(STATUS "Found Qt version ${QT_VERSION}.")

if(${QT_VERSION} VERSION_LESS 5.3.0)
  message(FATAL_ERROR "Found Qt version ${QT_VERSION} is too small.")
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_MODE 1)
else()
  set(DEBUG_MODE 0)
endif()

# To activate clazy: cmake . -DENABLE_CLAZY -DCMAKE_CXX_COMPILER=clazy
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}"
                                                  STREQUAL "GNU")
  if(ENABLE_CLAZY)
    string(
      CONCAT CLAZY_CHECKS
             "level0,"
             "no-fully-qualified-moc-types,"
             "no-lambda-in-connect,"
             "no-overloaded-signal,"
             "no-qenums,"
             "level1,"
             "no-connect-3arg-lambda,"
             "no-inefficient-qlist-soft,"
             "no-overridden-signal,"
             "no-qproperty-without-notify,"
             "no-range-loop,")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Xclang -plugin-arg-clazy -Xclang ${CLAZY_CHECKS} -Wno-deprecated-declarations"
    )
  endif()
endif()

#List and add all files by mask to the project, no manual enlisting is required.
file(GLOB qtask_SRCS
        src/*.h
        src/*.hpp
        src/*.cpp
        src/*.ui
    )

set(qtask_MOC_HEADERS src/configmanager.hpp)
set(qtask_RESOURCES res/qtask.qrc)
add_executable(qtask ${qtask_SRCS} ${qtask_RESOURCES})

target_link_libraries(qtask Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::GuiPrivate)
if (${QT_VERSION} VERSION_GREATER_EQUAL 5.15.0)
  target_link_libraries(qtask Qt${QT_VERSION_MAJOR}::Svg)
endif()

# Replacement of the QtSingleApplication for Qt5 and Qt6.
set(QAPPLICATION_CLASS
    QApplication
    CACHE STRING "Inheritance class for SingleApplication")
add_subdirectory(src/third-party/singleapplication)
target_link_libraries(qtask SingleApplication::SingleApplication)
target_link_libraries(qtask Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)


set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN_DIR})
configure_file(src/config.hpp.in ${GEN_DIR}/config.hpp)
set(GEN_H_LIST ${GEN_DIR}/config.hpp)
source_group(gen FILES ${GEN_H_LIST})
include_directories(${GEN_DIR})

include(GNUInstallDirs)
install(TARGETS qtask RUNTIME DESTINATION bin)
install(FILES res/icons/qtask.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/24x24/apps)
install(FILES res/qtask.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES res/icons/qtask.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
