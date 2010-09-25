

if(NOT CMAKE_CROSSCOMPILING)

add_definitions(-fshort-wchar)


else()

# Linking
link_directories("${REACTOS_SOURCE_DIR}/importlibs" ${REACTOS_BINARY_DIR}/lib/3rdparty/mingw)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS "-nodefaultlibs -nostdlib -Wl,--enable-auto-image-base -Wl,--kill-at -Wl,-T,${REACTOS_SOURCE_DIR}/global.lds")

# Compiler Core
add_definitions(-pipe -fms-extensions)

set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

# Debugging (Note: DWARF-4 on 4.5.1 when we ship)
add_definitions(-gdwarf-2 -g2 -femit-struct-debug-detailed=none -feliminate-unused-debug-types)

# Tuning
add_definitions(-march=pentium -mtune=i686)

# Warnings
add_definitions(-Wall -Wno-char-subscripts -Wpointer-arith -Wno-multichar -Wno-error=uninitialized -Wno-unused-value -Winvalid-pch)

# Optimizations
add_definitions(-Os -fno-strict-aliasing -ftracer -momit-leaf-frame-pointer -mpreferred-stack-boundary=2 -fno-set-stack-executable -fno-optimize-sibling-calls)

# C++ Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

# Macros
macro(set_entrypoint MODULE ENTRYPOINT)
  set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "-Wl,-entry,_${ENTRYPOINT}")
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
  set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "-Wl,--subsystem,${SUBSYSTEM}")
endmacro()

macro(add_importlibs MODULE)
  FOREACH(LIB ${ARGN})
    target_link_libraries(${MODULE} ${LIB}.a)
  ENDFOREACH()
endmacro()

macro(set_module_type MODULE TYPE)
  target_link_libraries(${MODULE} mingw_wmain mingw_common)
  if(${TYPE} MATCHES nativecui)
    set_subsystem(${MODULE} native)
    set_entrypoint(${MODULE} NtProcessStartup@4)
  endif()
  if(${TYPE} MATCHES win32gui)
    set_subsystem(${MODULE} windows)
    set_entrypoint(${MODULE} wWinMainCRTStartup)
  endif()
  if(${TYPE} MATCHES win32cui)
    set_subsystem(${MODULE} windows)
    set_entrypoint(${MODULE} mainCRTStartup)
  endif()
endmacro()

endif()

