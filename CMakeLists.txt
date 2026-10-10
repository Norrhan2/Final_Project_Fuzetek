cmake_minimum_required(VERSION 3.16)
project(EventTicketManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)   # single translation unit, no Q_OBJECT classes

option(BUILD_TESTS "Build the unit tests" ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# This project uses libpqxx 7.x (exec_params). libpqxx 8 removed it and needs C++20.
find_package(libpqxx CONFIG QUIET)
if(libpqxx_FOUND)
    if(libpqxx_VERSION VERSION_GREATER_EQUAL 8)
        message(FATAL_ERROR "libpqxx ${libpqxx_VERSION} found, but this project needs libpqxx 7.x")
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)
    if(PQXX_VERSION VERSION_GREATER_EQUAL 8)
        message(FATAL_ERROR "libpqxx ${PQXX_VERSION} found, but this project needs libpqxx 7.x")
    endif()
endif()

add_executable(EventTicketManager src/main.cpp)

target_link_libraries(EventTicketManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
if(TARGET libpqxx::pqxx)
    target_link_libraries(EventTicketManager PRIVATE libpqxx::pqxx)
else()
    target_link_libraries(EventTicketManager PRIVATE PkgConfig::PQXX)
endif()

if(WIN32)
    target_compile_definitions(EventTicketManager PRIVATE NOMINMAX)
endif()
if(MSVC)
    target_compile_options(EventTicketManager PRIVATE /utf-8)
endif()

# Unit tests: pure logic only (no Qt, no database).
if(BUILD_TESTS)
    enable_testing()
    add_executable(unit_tests tests/test_core.cpp)
    add_test(NAME unit_tests COMMAND unit_tests)
endif()