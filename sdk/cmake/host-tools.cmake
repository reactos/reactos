
function(setup_host_tools)
    file(MAKE_DIRECTORY ${REACTOS_BINARY_DIR}/host-tools)

    message(STATUS "Configuring host tools...")
    # cmake sets CC and CXX when those languages are enabled
    # so we need to clear them here
    execute_process(COMMAND
        ${CMAKE_COMMAND}
            -E env --unset=CC --unset=CXX
        ${CMAKE_COMMAND}
            -G "${CMAKE_GENERATOR}"
            -DARCH:STRING=${ARCH}
            ${USE_CLANG_CL_ARG}
            ${REACTOS_SOURCE_DIR}
        WORKING_DIRECTORY ${REACTOS_BINARY_DIR}/host-tools
        RESULT_VARIABLE _host_config_result
        OUTPUT_VARIABLE _host_config_log
        ERROR_VARIABLE  _host_config_log)

    # Show cmake output only if host-tools breaks
    if(NOT _host_config_result EQUAL 0)
        message("\nHost tools log:")
        message("${_host_config_log}")
        message(FATAL_ERROR "Failed to configure host tools")
    endif()

    set_property(SOURCE host_tools PROPERTY SYMBOLIC 1)

    # Make a host-tools target so it'll be built when needed
    # custom target + symbolic output prevents cmake from running
    # the command multiple times per build
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} --build ${REACTOS_BINARY_DIR}/host-tools
        OUTPUT host_tools)
    add_custom_target(build-host-tools ALL DEPENDS host_tools)

    include(${REACTOS_BINARY_DIR}/host-tools/ImportExecutables.cmake)
    include(${REACTOS_BINARY_DIR}/host-tools/TargetList.cmake)

    foreach(_target ${NATIVE_TARGETS})
        add_dependencies(native-${_target} build-host-tools)
    endforeach()

endfunction()
