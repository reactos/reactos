
include(ExternalProject)

function(setup_host_tools)
    list(APPEND HOST_TOOLS asmpp bin2c widl gendib cabman fatten hpp isohybrid mkhive mkisofs obj2bin spec2def geninc mkshelllink txt2nls utf16le xml2sdb)
    if(NOT MSVC)
        list(APPEND HOST_TOOLS pefixup)
        if (ARCH STREQUAL "i386")
            list(APPEND HOST_TOOLS rsym)
        endif()
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

    # Host tools have to run on this machine. The compiler in our environment targets
    # ARCH, which is fine whenever the host can execute ARCH binaries: an x86 or amd64
    # host runs i386 tools, an arm64 host runs all of them. Only ARM targets built on
    # an x86/amd64 host need a compiler pointed back at the host architecture.
    string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" HOST_CPU)
    if(MSVC AND (ARCH MATCHES "^arm") AND NOT (HOST_CPU MATCHES "^(arm|arm64|aarch64)$"))
        if(HOST_CPU MATCHES "^(amd64|x86_64)$")
            set(VCVARSALL_ARCH amd64)
        else()
            set(VCVARSALL_ARCH x86)
        endif()

        # Probe the known locations, instead of mapping each compiler version to a path.
        # VCINSTALLDIR ends with a path separator, so normalize it first.
        file(TO_CMAKE_PATH "$ENV{VCINSTALLDIR}" VCDIR)
        string(REGEX REPLACE "/+$" "" VCDIR "${VCDIR}")
        set(VCVARSALL "")
        foreach(CANDIDATE
                "${VCDIR}/Auxiliary/Build/vcvarsall.bat"  # VS 2017 and later
                "${VCDIR}/vcvarsall.bat")                 # VS 2015
            if(EXISTS "${CANDIDATE}")
                set(VCVARSALL "${CANDIDATE}")
                break()
            endif()
        endforeach()
        if(NOT VCVARSALL)
            message(FATAL_ERROR "Could not find vcvarsall.bat, which is needed to build the host tools "
                                "for ${ARCH}. Please configure from a Visual Studio command prompt.")
        endif()

        # Reuse the Windows SDK and toolset of this build, instead of whatever happens to
        # be the newest installed one. WindowsSDKVersion has a trailing backslash.
        string(REGEX REPLACE "\\\\$" "" WINDOWS_SDK_VERSION "$ENV{WindowsSDKVersion}")
        set(VCVARSALL_ARGS "${VCVARSALL_ARCH} ${WINDOWS_SDK_VERSION}")
        if(DEFINED ENV{VCToolsVersion})
            # Only VS 2017 and later can pin the toolset
            string(APPEND VCVARSALL_ARGS " -vcvars_ver=$ENV{VCToolsVersion}")
        endif()

        message("Compiling on ${HOST_CPU} for ${ARCH}, host tools use: vcvarsall ${VCVARSALL_ARGS}")

        # vcvarsall takes over a second and the shim runs on every build, so run it once
        # here and bake the variables the compiler and linker need into the shim.
        file(TO_NATIVE_PATH "${VCVARSALL}" VCVARSALL)
        set(VCVARS_CAPTURE "${REACTOS_BINARY_DIR}/host-tools/vcvars_capture.cmd")
        file(WRITE ${VCVARS_CAPTURE}
            "@set VSCMD_ARG_no_logo=1\n"
            "@set VSCMD_SKIP_SENDTELEMETRY=1\n"
            "@call \"${VCVARSALL}\" /clean_env\n"
            "@call \"${VCVARSALL}\" ${VCVARSALL_ARGS} || exit /b 1\n"
            "@set\n"
        )
        file(TO_NATIVE_PATH "${VCVARS_CAPTURE}" VCVARS_CAPTURE)
        execute_process(COMMAND cmd /c ${VCVARS_CAPTURE}
            OUTPUT_VARIABLE VCVARS_ENV
            RESULT_VARIABLE VCVARS_RESULT)
        if(NOT VCVARS_RESULT EQUAL 0)
            message(FATAL_ERROR "vcvarsall ${VCVARSALL_ARGS} failed:\n${VCVARS_ENV}")
        endif()

        set(SHIM_ENV "")
        foreach(VAR INCLUDE LIB LIBPATH "Path|PATH")
            if(NOT "\n${VCVARS_ENV}" MATCHES "\n(${VAR})=([^\r\n]*)")
                message(FATAL_ERROR "vcvarsall ${VCVARSALL_ARGS} did not set ${VAR}")
            endif()
            string(REPLACE "%" "%%" VALUE "${CMAKE_MATCH_2}")
            string(APPEND SHIM_ENV "@set \"${CMAKE_MATCH_1}=${VALUE}\"\n")
        endforeach()

        set(HOST_TOOLS_CMAKE_COMMAND "${REACTOS_BINARY_DIR}/host-tools/cmake_shim.cmd")
        file(WRITE ${HOST_TOOLS_CMAKE_COMMAND} "${SHIM_ENV}" "\"${CMAKE_COMMAND}\" %*\n")
    else()
        set(HOST_TOOLS_CMAKE_COMMAND "${CMAKE_COMMAND}")
    endif()

    # CMake might choose clang if it finds it in the PATH. Always prefer cl for host tools
    if (MSVC)
        list(APPEND CMAKE_HOST_TOOLS_EXTRA_ARGS
            -DCMAKE_C_COMPILER=cl
            -DCMAKE_CXX_COMPILER=cl)
    endif()

    if (MSVC_IDE)
        # Required for Bison/Flex wrappers created by /CMakeLists.txt.
        list(APPEND CMAKE_HOST_TOOLS_EXTRA_ARGS
            -DROS_SAVED_BISON_PKGDATADIR=${ROS_SAVED_BISON_PKGDATADIR}
            -DROS_SAVED_M4=${ROS_SAVED_M4}
            )
    endif()

    if(NOT DEFINED HOST_BUILD_TYPE)
        set(HOST_BUILD_TYPE Debug)
    endif()

    ExternalProject_Add(host-tools
        SOURCE_DIR ${REACTOS_SOURCE_DIR}
        PREFIX ${REACTOS_BINARY_DIR}/host-tools
        BINARY_DIR ${REACTOS_BINARY_DIR}/host-tools/bin
        CMAKE_COMMAND ${HOST_TOOLS_CMAKE_COMMAND}
        CMAKE_ARGS
            -UCMAKE_TOOLCHAIN_FILE
            -DARCH:STRING=${ARCH}
            -DCMAKE_INSTALL_PREFIX=${REACTOS_BINARY_DIR}/host-tools
            -DTOOLS_FOLDER=${REACTOS_BINARY_DIR}/host-tools/bin
            -DTARGET_COMPILER_ID=${CMAKE_C_COMPILER_ID}
            -DCMAKE_BUILD_TYPE=${HOST_BUILD_TYPE}
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
