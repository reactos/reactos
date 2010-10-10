
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    add_definitions(-D__i386__)
endif()

add_definitions(-Dinline=__inline)

if(NOT CMAKE_CROSSCOMPILING)



else()

add_definitions(/GS- /Zl /Zi)
add_definitions(-Dinline=__inline -D__STDC__=1)

set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")


macro(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        set(NEW_LINKER_FLAGS "/ENTRY:0")
    else()
        set(NEW_LINKER_FLAGS "/ENTRY:${ENTRYPOINT}")
    endif()
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
    set(NEW_LINKER_FLAGS "/subsystem:${SUBSYSTEM}")
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(add_importlibs MODULE)
    foreach(LIB ${ARGN})
        target_link_libraries(${MODULE} ${LIB}.LIB)
    endforeach()
endmacro()

macro(set_module_type MODULE TYPE)
    if(${TYPE} MATCHES nativecui)
        set_subsystem(${MODULE} native)
        add_importlibs(${MODULE} ntdll)
    endif()
    if (${TYPE} MATCHES win32gui)
        set_subsystem(${MODULE} windows)
    endif ()
    if (${TYPE} MATCHES win32cui)
        set_subsystem(${MODULE} console)
    endif ()
endmacro()

macro(set_unicode)
    add_definitions(-DUNICODE -D_UNICODE)
endmacro()

endif()

set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi  /Ob0 /Od")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi /Ob0 /Od")

macro(set_rc_compiler)
# dummy, this workaround is only needed in mingw due to lack of RC support in cmake
endmacro()

#typelib support
macro(ADD_TYPELIB TARGET)
    foreach(SOURCE ${ARGN})
        get_filename_component(FILE ${SOURCE} NAME_WE)
        set(OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.tlb)
        add_custom_command(OUTPUT ${OBJECT}
                           COMMAND midl /I ${REACTOS_SOURCE_DIR}/include/dxsdk /I . /I ${REACTOS_SOURCE_DIR}/include /I ${REACTOS_SOURCE_DIR}/include/psdk /win32 /tlb ${OBJECT} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
        list(APPEND OBJECTS ${OBJECT})
    endforeach()
    add_custom_target(${TARGET} ALL DEPENDS ${OBJECTS})
endmacro()
