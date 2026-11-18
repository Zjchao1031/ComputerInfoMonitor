if(NOT EXISTS "${MONITOR_CONFIG_DESTINATION}")
    if(NOT EXISTS "${MONITOR_CONFIG_SOURCE}")
        message(FATAL_ERROR "Configuration template does not exist: ${MONITOR_CONFIG_SOURCE}")
    endif()

    get_filename_component(monitor_config_directory "${MONITOR_CONFIG_DESTINATION}" DIRECTORY)
    file(MAKE_DIRECTORY "${monitor_config_directory}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy "${MONITOR_CONFIG_SOURCE}" "${MONITOR_CONFIG_DESTINATION}"
        RESULT_VARIABLE monitor_config_copy_result
    )

    if(NOT monitor_config_copy_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy configuration file to: ${MONITOR_CONFIG_DESTINATION}")
    endif()
endif()
