
function(configure_host_tools HOST_TOOLS_DIR)
    file(MAKE_DIRECTORY ${HOST_TOOLS_DIR})

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
        WORKING_DIRECTORY ${HOST_TOOLS_DIR}
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
        COMMAND ${CMAKE_COMMAND} --build ${HOST_TOOLS_DIR}
        OUTPUT host_tools)
    add_custom_target(build-host-tools ALL DEPENDS host_tools)

    include(${HOST_TOOLS_DIR}/ImportExecutables.cmake)
    include(${HOST_TOOLS_DIR}/TargetList.cmake)

    foreach(_target ${NATIVE_TARGETS})
        add_dependencies(native-${_target} build-host-tools)
    endforeach()

endfunction()

function(setup_host_tools)
    if(WITH_HOST_TOOLS)
        # Use pre-built tools, required for cross compiling with msvc
        # as only one target architecture is available at a time
        find_path(HOST_TOOLS_DIR
            NAMES ImportExecutables.cmake
            HINTS ${WITH_HOST_TOOLS} ${REACTOS_SOURCE_DIR}/${WITH_HOST_TOOLS}
            NO_CMAKE_PATH
            NO_CMAKE_ENVIRONMENT_PATH)
        message(STATUS "Using prebuilt host tools: ${HOST_TOOLS_DIR}")
        include(${HOST_TOOLS_DIR}/ImportExecutables.cmake)
    else()
        # Build host-tools. Changes to tool sources will rebuild targets
        # using that tool
        set(HOST_TOOLS_DIR ${REACTOS_BINARY_DIR}/host-tools)
        configure_host_tools(${HOST_TOOLS_DIR})
    endif()

endfunction()
