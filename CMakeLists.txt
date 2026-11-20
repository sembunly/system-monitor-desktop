
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
)

set(MONITOR_SOURCES
        src/monitor/SystemSampler.h
        src/monitor/SystemSampler.cpp
        src/monitor/cpu/CpuSampler.h
        src/monitor/cpu/CpuSampler.cpp
        src/monitor/ram/RamSampler.h
        src/monitor/ram/RamSampler.cpp
        src/monitor/disk/DiskSampler.h
        src/monitor/disk/DiskSampler.cpp

        src/ui/MainWindow.h
        src/ui/MainWindow.cpp

        src/ui/pages/DashboardPage.h
        src/ui/pages/DashboardPage.cpp

        src/ui/widgets/MetricCard.h
        src/ui/widgets/MetricCard.cpp
        src/ui/widgets/LiveChart.h
        src/ui/widgets/LiveChart.cpp
        src/ui/widgets/CoreBarChart.h
        src/ui/widgets/CoreBarChart.cpp
)

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    list(APPEND MONITOR_SOURCES src/monitor/MacAppIcon.h src/monitor/MacAppIcon.mm)
    set_source_files_properties(src/monitor/MacAppIcon.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    set(MONITOR_PLATFORM_LIBS "${COCOA_FRAMEWORK}")
endif()

add_executable(SystemMonitor src/main.cpp ${MONITOR_SOURCES})

target_link_libraries(SystemMonitor PRIVATE
        Qt6::Core
        Qt6::Widgets
        ${MONITOR_PLATFORM_LIBS}
)

if(APPLE)
    set_target_properties(SystemMonitor PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "SEM Bunly - System Monitor"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.systemmonitor.desktop"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
elseif(WIN32)
    set_target_properties(SystemMonitor PROPERTIES
            WIN32_EXECUTABLE TRUE
    )
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(MonitorTests tests/MonitorTests.cpp ${MONITOR_SOURCES})
    target_include_directories(MonitorTests PRIVATE src)
    target_link_libraries(MonitorTests PRIVATE Qt6::Core Qt6::Widgets ${MONITOR_PLATFORM_LIBS})
    add_test(NAME MonitorTests COMMAND MonitorTests)
    set_tests_properties(MonitorTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 20
    )
endif()
