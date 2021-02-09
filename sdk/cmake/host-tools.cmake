
include(ExternalProject)

function(setup_host_tools)
    list(APPEND HOST_TOOLS bin2c widl gendib cabman fatten hpp isohybrid mkhive mkisofs obj2bin spec2def geninc mkshelllink utf16le xml2sdb)
    if(NOT MSVC)
        list(APPEND HOST_TOOLS rsym pefixup)
    endif()
    list(TRANSFORM HOST_TOOLS PREPEND "${REACTOS_BINARY_DIR}/host-tools/bin/" OUTPUT_VARIABLE HOST_TOOLS_OUTPUT)
    if (CMAKE_HOST_WIN32)
        list(TRANSFORM HOST_TOOLS_OUTPUT APPEND ".exe")
        if(MSVC_IDE)
            set(HOST_EXTRA_DIR "$(ConfigurationName)/")
        endif()
        set(HOST_EXE_SUFFIX ".exe")
    endif()

    ExternalProject_Add(host-tools
        SOURCE_DIR ${REACTOS_SOURCE_DIR}
        PREFIX ${REACTOS_BINARY_DIR}/host-tools
        BINARY_DIR ${REACTOS_BINARY_DIR}/host-tools/bin
        CMAKE_ARGS -UCMAKE_TOOLCHAIN_FILE -DARCH:STRING=${ARCH} -DCMAKE_INSTALL_PREFIX=${REACTOS_BINARY_DIR}/host-tools -DTOOLS_FOLDER=${REACTOS_BINARY_DIR}/host-tools/bin
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
endfunction()
