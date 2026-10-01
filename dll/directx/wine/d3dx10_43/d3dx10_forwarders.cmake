
function(add_d3dx10_forwarder VERSION)
    set(_target d3dx10_${VERSION})

    spec2def(${_target}.dll ${_target}.spec ADD_IMPORTLIB)

    add_library(${_target} MODULE
        ${_target}_main.c
        version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/${_target}_stubs.c
        ${CMAKE_CURRENT_BINARY_DIR}/${_target}.def)

    target_compile_definitions(${_target} PRIVATE
        __WINESRC__
        __ROS_LONG64__)

    target_include_directories(${_target} BEFORE PRIVATE
        ${REACTOS_SOURCE_DIR}/sdk/include/wine
        ${REACTOS_BINARY_DIR}/sdk/include/wine
        ${CMAKE_CURRENT_BINARY_DIR})

    set_module_type(${_target} win32dll)
    target_link_libraries(${_target} wine)

    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/${_target}.spec _spec_forwards
         REGEX "d3dx10_[0-9]+\\.")
    set(_forward_targets "")
    foreach(_line IN LISTS _spec_forwards)
        string(REGEX MATCH "d3dx10_[0-9]+" _forward "${_line}")
        if(_forward AND NOT _forward STREQUAL ${_target})
            list(APPEND _forward_targets ${_forward})
        endif()
    endforeach()
    if(_forward_targets)
        list(REMOVE_DUPLICATES _forward_targets)
    endif()

    add_importlibs(${_target} ${_forward_targets} msvcrt kernel32 ntdll)
    add_dependencies(${_target} wineheaders d3d_idl_headers)
    add_cd_file(TARGET ${_target} DESTINATION reactos/system32 FOR all)
endfunction()
