
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
  add_definitions(-D__i386__)
endif()

add_definitions(-Dinline=__inline)

if(NOT CMAKE_CROSSCOMPILING)



else()

add_definitions(/GS- /Zl /Zi)
add_definitions(-Dinline=__inline -D__STDC__=1)

macro(set_entrypoint MODULE ENTRYPOINT)
    set(NEW_LINKER_FLAGS "/ENTRY:${ENTRYPOINT}")
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
  FOREACH(LIB ${ARGN})
    target_link_libraries(${MODULE} ${LIB}.LIB)
  ENDFOREACH()
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

macro(set_unicode MODULE STATE)
   if(${STATE} MATCHES yes)
       add_definitions(-DUNICODE -D_UNICODE)
   endif()
endmacro()

endif()

set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi  /Ob0 /Od")
SET(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi /Ob0 /Od")

macro(set_rc_compiler)
# dummy, this workaround is only needed in mingw due to lack of RC support in cmake
endmacro()