cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(qtask_tests CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

#Tests
file(GLOB_RECURSE TESTS_LIST
     ${CMAKE_CURRENT_LIST_DIR}/*.h
     ${CMAKE_CURRENT_LIST_DIR}/*.hpp
     ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    )
list(LENGTH TESTS_LIST TESTS_LIST_FILES_COUNT)
if (TESTS_LIST_FILES_COUNT GREATER 0)
    find_package(GTest REQUIRED)

    #Detect best qt version in the system.
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
    if (${QT_VERSION} VERSION_LESS 5.15.0)
      find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)
    else()
      #Not sure, if should search/link GuiPrivate Svg with old versions.
      find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets GuiPrivate Svg)
    endif()

    message(STATUS "Found Qt version for tests: ${QT_VERSION}.")


    source_group("tests" FILES ${TESTS_LIST})
    add_executable(qtask_tests
                   ${TESTS_LIST}
    )
    target_link_libraries(qtask_tests PRIVATE
                        gtest
                        gmock
                        Qt${QT_VERSION_MAJOR}::Widgets
    )
    target_include_directories(qtask_tests PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR}
                        ${CMAKE_CURRENT_LIST_DIR}/../src
                    )
    add_test(NAME qtask_tests COMMAND qtask_tests)
endif()
