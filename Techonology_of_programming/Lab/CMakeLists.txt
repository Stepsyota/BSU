cmake_minimum_required(VERSION 3.16)
project(elevators LANGUAGES CXX)

# CLion по умолчанию крутит Ninja -j по числу ядер. Каждый g++ с Qt легко
# берёт сотни МБ, на 12 ГБ это валит IDE. Пул режет параллелизм независимо от -j.
set_property(GLOBAL PROPERTY JOB_POOLS compile=2 link=1)
set(CMAKE_JOB_POOL_COMPILE compile)
set(CMAKE_JOB_POOL_LINK link)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

if(APPLE)
    list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

add_executable(elevators
    main.cpp
    src/domain/src/Person.cpp
    src/domain/src/Elevator.cpp
    src/domain/src/MinWaitStrategy.cpp
    src/domain/src/MinIdleStrategy.cpp
    src/domain/src/Scenario.cpp
    src/domain/src/Simulation.cpp
    src/io/src/JsonStore.cpp
    src/ui/src/MainWindow.cpp
    src/ui/src/ConfigDialog.cpp
    src/ui/src/StatsDialog.cpp
    src/ui/src/BuildingView.cpp
    src/ui/include/MainWindow.h
    src/ui/include/ConfigDialog.h
    src/ui/include/StatsDialog.h
)

target_include_directories(elevators PRIVATE
    src/domain/include
    src/io/include
    src/ui/include
)

target_link_libraries(elevators PRIVATE Qt6::Widgets)
