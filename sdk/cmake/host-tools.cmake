
include(ExternalProject)

function(setup_host_tools)
    list(APPEND HOST_TOOLS bin2c widl gendib cabman fatten hpp isohybrid mkhive mkisofs obj2bin spec2def geninc mkshelllink txt2nls utf16le xml2sdb)
    if(NOT MSVC)
        list(APPEND HOST_TOOLS rsym pefixup)
    endif()
    if ((ARCH STREQUAL "amd64") AND (CMAKE_C_COMPILER_ID STREQUAL "GNU"))
        execute_process(
            COMMAND ${CMAKE_C_COMPILER} --print-file-name=plugin
            OUTPUT_VARIABLE GCC_PLUGIN_DIR)
        string(STRIP ${GCC_PLUGIN_DIR} GCC_PLUGIN_DIR)
        list(APPEND CMAKE_HOST_TOOLS_EXTRA_ARGS -DGCC_PLUGIN_DIR=${GCC_PLUGIN_DIR})
        list(APPEND HOST_MODULES gcc_plugin_seh)
        if (CMAKE_HOST_WIN32)
            list(APPEND HOST_MODULES g++_plugin_seh)
        endif()
    endif()
    list(TRANSFORM HOST_TOOLS PREPEND "${REACTOS_BINARY_DIR}/host-tools/bin/" OUTPUT_VARIABLE HOST_TOOLS_OUTPUT)
    if (CMAKE_HOST_WIN32)
        list(TRANSFORM HOST_TOOLS_OUTPUT APPEND ".exe")
        if(MSVC_IDE)
            set(HOST_EXTRA_DIR "$(ConfigurationName)/")
        endif()
        set(HOST_EXE_SUFFIX ".exe")
        set(HOST_MODULE_SUFFIX ".dll")
    else()
        set(HOST_MODULE_SUFFIX ".so")
    endif()

    # CMake might choose clang if it finds it in the PATH. Always prefer cl for host tools
    if (MSVC)
        list(APPEND CMAKE_HOST_TOOLS_EXTRA_ARGS
            -DCMAKE_C_COMPILER=cl
            -DCMAKE_CXX_COMPILER=cl)
    endif()

    ExternalProject_Add(host-tools
        SOURCE_DIR ${REACTOS_SOURCE_DIR}
        PREFIX ${REACTOS_BINARY_DIR}/host-tools
        BINARY_DIR ${REACTOS_BINARY_DIR}/host-tools/bin
        CMAKE_ARGS
            -UCMAKE_TOOLCHAIN_FILE
            -DARCH:STRING=${ARCH}
            -DCMAKE_INSTALL_PREFIX=${REACTOS_BINARY_DIR}/host-tools
            -DTOOLS_FOLDER=${REACTOS_BINARY_DIR}/host-tools/bin
            -DROS_SAVED_M4=${ROS_SAVED_M4}
            -DROS_SAVED_BISON_PKGDATADIR=${ROS_SAVED_BISON_PKGDATADIR}
            -DTARGET_COMPILER_ID=${CMAKE_C_COMPILER_ID}
            ${CMAKE_HOST_TOOLS_EXTRA_ARGS}
        BUILD_ALWAYS TRUE
        INSTALL_COMMAND ${CMAKE_COMMAND} -E true
        BUILD_BYPRODUCTS ${HOST_TOOLS_OUTPUT}
    )

    ExternalProject_Get_Property(host-tools INSTALL_DIR)

    foreach(_tool ${HOST_TOOLS})
        add_executable(native-${_tool} IMPORTED)
        set_target_properties(native-${_tool} PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/bin/${HOST_EXTRA_DIR}${_tool}${HOST_EXE_SUFFIX})
        add_dependencies(native-${_tool} host-tools ${INSTALL_DIR}/bin/${HOST_EXTRA_DIR}${_tool}${HOST_EXE_SUFFIX})
    endforeach()

    foreach(_module ${HOST_MODULES})
        add_library(native-${_module} MODULE IMPORTED)
        set_target_properties(native-${_module} PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/bin/${HOST_EXTRA_DIR}${_module}${HOST_MODULE_SUFFIX})
        add_dependencies(native-${_module} host-tools ${INSTALL_DIR}/bin/${HOST_EXTRA_DIR}${_module}${HOST_MODULE_SUFFIX})
    endforeach()
endfunction()
