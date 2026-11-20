
cmake_minimum_required(VERSION 3.21)

project(SystemMonitor
        VERSION 0.1.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Widgets
        Sql
        Charts
)

add_executable(SystemMonitor
        src/main.cpp

        src/ui/MainWindow.h
        src/ui/MainWindow.cpp

        src/ui/pages/DashboardPage.h
        src/ui/pages/DashboardPage.cpp

        src/ui/widgets/MetricCard.h
        src/ui/widgets/MetricCard.cpp
)


target_link_libraries(SystemMonitor PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Charts
)

if(APPLE)
    set_target_properties(SystemMonitor PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "SEM Bunly - System Monitor"
    )
elseif(WIN32)
    set_target_properties(SystemMonitor PROPERTIES
            WIN32_EXECUTABLE TRUE
    )
endif()