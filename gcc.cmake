

if(NOT CMAKE_CROSSCOMPILING)

add_definitions(-fshort-wchar)


else()

# Linking
link_directories("${REACTOS_SOURCE_DIR}/importlibs" ${REACTOS_BINARY_DIR}/lib/3rdparty/mingw)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS "-nodefaultlibs -nostdlib -Wl,--enable-auto-image-base -Wl,--kill-at")
# -Wl,-T,${REACTOS_SOURCE_DIR}/global.lds

# Compiler Core
add_definitions(-pipe -fms-extensions)

set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

# Debugging (Note: DWARF-4 on 4.5.1 when we ship)
#add_definitions(-gdwarf-2 -g2 -femit-struct-debug-detailed=none -feliminate-unused-debug-types)

# Tuning
add_definitions(-march=pentium -mtune=i686)

# Warnings
add_definitions(-Wall -Wno-char-subscripts -Wpointer-arith -Wno-multichar -Wno-error=uninitialized -Wno-unused-value -Winvalid-pch)

# Optimizations
add_definitions(-Os -fno-strict-aliasing -ftracer -momit-leaf-frame-pointer -mpreferred-stack-boundary=2 -fno-set-stack-executable -fno-optimize-sibling-calls)

#linkage hell...
add_library(gcc STATIC IMPORTED)
set_target_properties(gcc PROPERTIES IMPORTED_LOCATION ${REACTOS_SOURCE_DIR}/importlibs/libgcc.a
    IMPORTED_LINK_INTERFACE_LIBRARIES "mingw_common -lkernel32")
add_library(supc++ STATIC IMPORTED)
set_target_properties(supc++ PROPERTIES IMPORTED_LOCATION ${REACTOS_SOURCE_DIR}/importlibs/libsupc++.a
    IMPORTED_LINK_INTERFACE_LIBRARIES "gcc -lmsvcrt")

# Macros
macro(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        set(NEW_LINKER_FLAGS "-Wl,-entry,0")
    else()
        set(NEW_LINKER_FLAGS "-Wl,-entry,_${ENTRYPOINT}")
    endif()
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
  foreach(LIB ${ARGN})
    target_link_libraries(${MODULE} ${LIB}.dll.a)
  endforeach()
endmacro()

macro(set_module_type MODULE TYPE)

    add_dependencies(${MODULE} psdk buildno_header)
  
    if(${TYPE} MATCHES nativecui)
        set_subsystem(${MODULE} native)
        set_entrypoint(${MODULE} NtProcessStartup@4)
    endif()
    if(${TYPE} MATCHES win32gui)
        set_subsystem(${MODULE} windows)
        set_entrypoint(${MODULE} WinMainCRTStartup)
        if(NOT IS_UNICODE)
            target_link_libraries(${MODULE} mingw_main)
        else()
            target_link_libraries(${MODULE} mingw_wmain)
        endif(NOT IS_UNICODE)
        target_link_libraries(${MODULE} mingw_common gcc)
    endif()
    if(${TYPE} MATCHES win32cui)
        set_subsystem(${MODULE} console)
        set_entrypoint(${MODULE} mainCRTStartup)
        if(NOT IS_UNICODE)
            target_link_libraries(${MODULE} mingw_main)
        else()
            target_link_libraries(${MODULE} mingw_wmain)
        endif(NOT IS_UNICODE)
        target_link_libraries(${MODULE} mingw_common gcc)
    endif()
    if(${TYPE} MATCHES win32dll)
        set_entrypoint(${MODULE} DllMain@12)
    endif()
    if(${TYPE} MATCHES win32ocx)
        set_entrypoint(${MODULE} DllMain@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
    endif()
endmacro()

endif()

macro(set_unicode)
   add_definitions(-DUNICODE -D_UNICODE)
   set(IS_UNICODE 1)
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

    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${result_defs} ${result_incs} -i <SOURCE> -O coff -o <OBJECT>")
endmacro()

#typelib support
macro(ADD_TYPELIB TARGET)
  foreach(SOURCE ${ARGN})
    get_filename_component(FILE ${SOURCE} NAME_WE)
    set(OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.tlb)
    add_custom_command(OUTPUT ${OBJECT}
                       COMMAND native-widl -I${REACTOS_SOURCE_DIR}/include/dxsdk -I. -I${REACTOS_SOURCE_DIR}/include -I${REACTOS_SOURCE_DIR}/include/psdk -m32 --win32 -t -T ${OBJECT} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
                       DEPENDS native-widl)
    list(APPEND OBJECTS ${OBJECT})
  endforeach()
  add_custom_target(${TARGET} ALL DEPENDS ${OBJECTS})
endmacro()
