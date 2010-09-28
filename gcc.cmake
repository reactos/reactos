

if(NOT CMAKE_CROSSCOMPILING)

add_definitions(-fshort-wchar)


else()

# Linking
link_directories("${REACTOS_SOURCE_DIR}/importlibs" ${REACTOS_BINARY_DIR}/lib/3rdparty/mingw)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> -lstdc++ -lsupc++ -lgcc -lmingwex -lmingw32 <LINK_LIBRARIES>")
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
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

# Macros
macro(set_entrypoint MODULE ENTRYPOINT)
    set(NEW_LINKER_FLAGS "-Wl,-entry,_${ENTRYPOINT}")
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
    set(NEW_LINKER_FLAGS "-Wl,--subsystem,${SUBSYSTEM}")
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(add_importlibs MODULE)
  FOREACH(LIB ${ARGN})
    target_link_libraries(${MODULE} ${LIB}.a)
  ENDFOREACH()
endmacro()

macro(set_module_type MODULE TYPE)

  add_dependencies(${MODULE} builno_header psdk)
  
  if(${TYPE} MATCHES nativecui)
    set_subsystem(${MODULE} native)
    set_entrypoint(${MODULE} NtProcessStartup@4)
  endif()
  if(${TYPE} MATCHES win32gui)
    set_subsystem(${MODULE} windows)
    set_entrypoint(${MODULE} WinMainCRTStartup)
  endif()
  if(${TYPE} MATCHES win32cui)
    set_subsystem(${MODULE} console)
    set_entrypoint(${MODULE} mainCRTStartup)
  endif()
  if(${TYPE} MATCHES win32dll)
    target_link_libraries(${MODULE} mingw_dllmain mingw_common)
    set_entrypoint(${MODULE} DllMain@12)
  endif()
endmacro()

endif()

macro(set_unicode MODULE STATE)
   if(${STATE} MATCHES yes)
       add_definitions(-DUNICODE -D_UNICODE)
       target_link_libraries(${MODULE} mingw_wmain)
   else()
       target_link_libraries(${MODULE} mingw_main)
   endif()
   
  target_link_libraries(${MODULE} mingw_common)
endmacro()

# Workaround lack of mingw RC support in cmake
macro(set_rc_compiler)
    get_directory_property(defines COMPILE_DEFINITIONS)
    get_directory_property(includes INCLUDE_DIRECTORIES)

    foreach(arg ${defines})
        set(result_defs "${result_defs} -D${arg}")
    endforeach(arg ${defines})

    foreach(arg ${includes})
        set(result_incs "-I${arg} ${result_incs}")
    endforeach(arg ${includes})

    SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${result_defs} ${result_incs} -i <SOURCE> -O coff -o <OBJECT>")
endmacro()

#typelib support
macro(ADD_TYPELIB TARGET)
  FOREACH(SOURCE ${ARGN})
    GET_FILENAME_COMPONENT(FILE ${SOURCE} NAME_WE)
    SET(OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.tlb)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${OBJECT}
      COMMAND native-widl -I${REACTOS_SOURCE_DIR}/include/dxsdk -I. -I${REACTOS_SOURCE_DIR}/include -I${REACTOS_SOURCE_DIR}/include/psdk -m32 --win32 -t -T ${OBJECT} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
      DEPENDS native-widl
    )
    LIST(APPEND OBJECTS ${OBJECT})
  ENDFOREACH()
  ADD_CUSTOM_TARGET(${TARGET} ALL DEPENDS ${OBJECTS})
ENDMACRO()
