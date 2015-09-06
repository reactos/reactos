
if(CMAKE_HOST_WIN32)
    set(native_suffix ".exe")
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} _build_type)

# List of host tools
list(APPEND host_tools_list bin2c widl gendib cabman cdmake mkhive obj2bin spec2def geninc mkshelllink utf16le)
if(NOT MSVC)
    list(APPEND host_tools_list rsym)
endif()

foreach(_host_tool ${host_tools_list})
    if(MSVC_IDE)
        get_filename_component(_tool_location "${CMAKE_CURRENT_BINARY_DIR}/host-tools/${CMAKE_BUILD_TYPE}/${_host_tool}${native_suffix}" ABSOLUTE)
    else()
        get_filename_component(_tool_location "${CMAKE_CURRENT_BINARY_DIR}/host-tools/${_host_tool}${native_suffix}" ABSOLUTE)
    endif()
    list(APPEND tools_binaries ${_tool_location})
    add_executable(native-${_host_tool} IMPORTED)
    set_property(TARGET native-${_host_tool} PROPERTY IMPORTED_LOCATION_${_build_type} ${_tool_location})
    add_dependencies(native-${_host_tool} host-tools)
endforeach()

include(ExternalProject)

ExternalProject_Add(host-tools
    SOURCE_DIR ${REACTOS_SOURCE_DIR}
    BINARY_DIR ${REACTOS_BINARY_DIR}/host-tools
    STAMP_DIR ${REACTOS_BINARY_DIR}/host-tools/stamps
    BUILD_ALWAYS 1
    PREFIX host-tools
    EXCLUDE_FROM_ALL 1
    CMAKE_ARGS "-DNEW_STYLE_BUILD=1"
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${tools_binaries})
