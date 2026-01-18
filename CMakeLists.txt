cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(qtask CXX)

set(QTASK_VERSION_MAJOR 1) # something big added / changed
set(QTASK_VERSION_MINOR 2) # new small feature added
set(QTASK_VERSION_PATCH 3) # refactor/bug fix inside current minor

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


option(BUILD_TESTS "Build tests." OFF)

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

# List of Qt components we will need to find. Note, there is linking list below yet.
set(QT_COMPONENTS_WE_NEED Core Widgets Concurrent Svg)

# Detect best qt version in the system.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS_WE_NEED})
message(STATUS "Found Qt version ${QT_VERSION}.")
if(${QT_VERSION} VERSION_LESS 5.3.0)
  message(FATAL_ERROR "Found Qt version ${QT_VERSION} is too small.")
endif()
# Bind tools to the version if both found, list must be the same in 2 calls.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_COMPONENTS_WE_NEED})

#Private package is different on distributions
if(TARGET Qt${QT_VERSION_MAJOR}::GuiPrivate)
    # Debian just adds it silently.
    message(STATUS "Qt${QT_VERSION_MAJOR}::GuiPrivate is already defined by system.")
    set(Qt${QT_VERSION_MAJOR}GuiPrivate_FOUND TRUE)
else()
    #Arch linux, we need explicit search.
    message(STATUS "Checking for dedicated package Qt${QT_VERSION_MAJOR}::GuiPrivate.")
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS GuiPrivate QUIET)
endif()

# Fallback if it was nor explicit, or silent added.
if(NOT TARGET Qt${QT_VERSION_MAJOR}::GuiPrivate)

    set(QT_PRIVATE_INCLUDE_DIRS "")
    message(STATUS "Searching for Qt ${QT_VERSION} private headers on disk...")

    # Option 1
    set(_check_targets Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui)
    set(_all_qt_dirs "")

    foreach(_tgt ${_check_targets})
        if(TARGET ${_tgt})
            get_target_property(_inc ${_tgt} INTERFACE_INCLUDE_DIRECTORIES)
            if(_inc)
                list(APPEND _all_qt_dirs ${_inc})
            endif()
        endif()
    endforeach()

    # Option 2
    if(NOT QT_PRIVATE_INCLUDE_DIRS)
        find_path(_core_private_root
            NAMES private/qglobal_p.h
            PATH_SUFFIXES "QtCore/${QT_VERSION}" "qt6/QtCore/${QT_VERSION}" "qt5/QtCore/${QT_VERSION}"
            PATHS "/usr/include" "/usr/include/${CMAKE_LIBRARY_ARCHITECTURE}" "/usr/local/include"
        )

        if(_core_private_root)
            message(STATUS "  [HUNT] Found private root via disk search: ${_core_private_root}")
            get_filename_component(_qt_inc_dir "${_core_private_root}" DIRECTORY)
            get_filename_component(_qt_inc_dir "${_qt_inc_dir}" DIRECTORY)

            foreach(_mod QtCore QtGui QtWidgets)
                string(REPLACE "Qt" "" _mod_plain "${_mod}")
                set(_p "${_qt_inc_dir}/${_mod_plain}/${QT_VERSION}")
                if(EXISTS "${_p}/${_mod_plain}/private")
                    list(APPEND QT_PRIVATE_INCLUDE_DIRS "${_p}" "${_p}/${_mod_plain}")
                endif()
            endforeach()
        endif()
    endif()
    # Option 3
    if(NOT QT_PRIVATE_INCLUDE_DIRS AND _all_qt_dirs)
        list(REMOVE_DUPLICATES _all_qt_dirs)
        foreach(_dir ${_all_qt_dirs})
            file(GLOB _subdirs RELATIVE "${_dir}" "${_dir}/Qt*")
            foreach(_subdir ${_subdirs})
                set(_potential_base "${_dir}/${_subdir}/${QT_VERSION}")
                if(EXISTS "${_potential_base}/${_subdir}/private")
                    list(APPEND QT_PRIVATE_INCLUDE_DIRS "${_potential_base}" "${_potential_base}/${_subdir}")
                endif()
            endforeach()
        endforeach()
    endif()

    if(QT_PRIVATE_INCLUDE_DIRS)
        list(REMOVE_DUPLICATES QT_PRIVATE_INCLUDE_DIRS)
        if(NOT TARGET Qt${QT_VERSION_MAJOR}::GuiPrivate)
            add_library(Qt${QT_VERSION_MAJOR}::GuiPrivate INTERFACE IMPORTED)
            target_link_libraries(Qt${QT_VERSION_MAJOR}::GuiPrivate INTERFACE Qt${QT_VERSION_MAJOR}::Gui)
            target_include_directories(Qt${QT_VERSION_MAJOR}::GuiPrivate SYSTEM INTERFACE ${QT_PRIVATE_INCLUDE_DIRS})
        endif()
        set(Qt${QT_VERSION_MAJOR}GuiPrivate_FOUND TRUE)
        message(STATUS "Found private headers: ${QT_PRIVATE_INCLUDE_DIRS}")
    else()
        message(FATAL_ERROR "Could not find Qt ${QT_VERSION} private headers.
        Make sure qt6-base-private-dev is installed.
        Checked architecture: ${CMAKE_LIBRARY_ARCHITECTURE}")
    endif()
endif()

if(NOT Qt${QT_VERSION_MAJOR}GuiPrivate_FOUND)
message(FATAL_ERROR "Could not find Qt${QT_VERSION_MAJOR}GuiPrivate, check of package like qt${QT_VERSION_MAJOR}-base-private-dev is installed.")
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


# Debug / Release configuration is here
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_MODE 1)
else()
  set(DEBUG_MODE 0)
  add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

# Replacement of the QtSingleApplication for Qt5 and Qt6.
set(QAPPLICATION_CLASS
    QApplication
    CACHE STRING "Inheritance class for SingleApplication")
add_subdirectory(src/third-party/singleapplication)

# Linking Qt components we found. This is a little tricky depending on major version used.
target_link_libraries(qtask
             Qt::Core
             Qt::Widgets
             Qt::Concurrent
             SingleApplication::SingleApplication
             Qt${QT_VERSION_MAJOR}::GuiPrivate # "Private" requires explicit version mention.
)
# Fallback to the historical code, linkage was not needed all years before. Cannot check in 2025.
# Delete if () in case you have problems with missing Svg module.
if (${QT_VERSION} VERSION_GREATER_EQUAL 5.15.0)
  target_link_libraries(qtask Qt::Svg)
endif()


target_link_libraries(qtask SingleApplication::SingleApplication)


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
