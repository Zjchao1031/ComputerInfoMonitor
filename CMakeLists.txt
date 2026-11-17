cmake_minimum_required(VERSION 3.21)

project(ComputerInfoMonitor
    VERSION 0.1.0
    DESCRIPTION "i.MX6ULL computer information monitor"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Network Widgets)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

qt_add_executable(computer-info-monitor
    resources/resources.qrc
    src/main.cpp
    include/core/app_config.h
    include/core/fixed_parameters.h
    include/core/monitor_service.h
    include/mock/mock_monitor_data.h
    include/model/history_series_ids.h
    include/model/monitor_snapshot.h
    include/ui/connection_status_widget.h
    include/ui/cpu_detail_page.h
    include/ui/cpu_sensor_tree.h
    include/ui/dashboard_page.h
    include/ui/detail_header.h
    include/ui/gpu_detail_page.h
    include/ui/history_chart.h
    include/ui/main_window.h
    include/ui/memory_detail_page.h
    include/ui/metric_field.h
    include/ui/monitor_card.h
    include/ui/theme.h
    src/core/app_config.cpp
    src/core/monitor_service.cpp
    src/mock/mock_monitor_data.cpp
    src/ui/connection_status_widget.cpp
    src/ui/cpu_detail_page.cpp
    src/ui/cpu_sensor_tree.cpp
    src/ui/dashboard_page.cpp
    src/ui/detail_header.cpp
    src/ui/gpu_detail_page.cpp
    src/ui/history_chart.cpp
    src/ui/main_window.cpp
    src/ui/memory_detail_page.cpp
    src/ui/metric_field.cpp
    src/ui/monitor_card.cpp
    src/ui/theme.cpp
)

target_link_libraries(computer-info-monitor
    PRIVATE
        Qt6::Network
        Qt6::Widgets
)

target_include_directories(computer-info-monitor
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(MSVC)
    target_compile_options(computer-info-monitor PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(computer-info-monitor PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(computer-info-monitor PROPERTIES
    OUTPUT_NAME "computer-info-monitor"
)

install(TARGETS computer-info-monitor
    RUNTIME DESTINATION bin
)

add_custom_command(TARGET computer-info-monitor POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config/monitor.ini"
        "$<TARGET_FILE_DIR:computer-info-monitor>/monitor.ini"
)

install(FILES config/monitor.ini
    DESTINATION bin
)

