
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
  add_definitions(-D__i386__)
endif()

add_definitions(-Dinline=__inline)

if(NOT CMAKE_CROSSCOMPILING)



else()

add_definitions(/GS- /Zl /Zi)
add_definitions(-Dinline=__inline -D__STDC__=1)

macro(set_entrypoint MODULE ENTRYPOINT)
  set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "/ENTRY:${ENTRYPOINT}")
endmacro()

macro(add_importlibs MODULE)
  FOREACH(LIB ${ARGN})
    target_link_libraries(${MODULE} ${LIB}.LIB)
  ENDFOREACH()
endmacro()

macro(set_module_type MODULE TYPE)
  if (${TYPE} MATCHES win32gui)
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "/subsystem:windows")
  endif ()
endmacro()

endif()
