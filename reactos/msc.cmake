
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    # no optimitation
elseif(OPTIMIZE STREQUAL "1")
    add_definitions(/O1)
elseif(OPTIMIZE STREQUAL "2")
    add_definitions(/O2)
elseif(OPTIMIZE STREQUAL "3")
    add_definitions(/Ot /Ox /GS-)
elseif(OPTIMIZE STREQUAL "4")
    add_definitions(/Os /Ox /GS-)
elseif(OPTIMIZE STREQUAL "5")
    add_definitions(/GF /Gy /Ob2 /Os /Ox /GS-)
endif()

add_definitions(/X /GR- /GS- /Zl)
add_definitions(-Dinline=__inline -D__STDC__=1)

if(${_MACHINE_ARCH_FLAG} MATCHES X86)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
endif()

if(${ARCH} MATCHES amd64)
    add_definitions(-D__x86_64)
endif()

link_directories("${REACTOS_BINARY_DIR}/importlibs" ${REACTOS_BINARY_DIR}/lib/3rdparty/mingw)

set(CMAKE_RC_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_ASM_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})

macro(add_pch _target_name _header_filename _src_list)
    get_filename_component(FILE ${_header_filename} NAME)
    set(_gch_filename "${_target_name}_${FILE}.gch")
    add_custom_command(
        OUTPUT ${_gch_filename}
        COMMAND echo Ignoring precompiled header
        DEPENDS ${_header_filename})
endmacro()

macro(add_linkerflag MODULE _flag)
    set(NEW_LINKER_FLAGS ${_flag})
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        add_linkerflag(${MODULE} "/NOENTRY")
    else()
        add_linkerflag(${MODULE} "/ENTRY:${ENTRYPOINT}")
    endif()
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
    add_linkerflag(${MODULE} "/subsystem:${SUBSYSTEM}")
endmacro()

macro(set_image_base MODULE IMAGE_BASE)
    add_linkerflag(${MODULE} "/BASE:${IMAGE_BASE}")
endmacro()

macro(set_module_type MODULE TYPE)
    add_dependencies(${MODULE} psdk)
    if(${TYPE} MATCHES nativecui)
        set_subsystem(${MODULE} native)
        set_entrypoint(${MODULE} NtProcessStartup@4)
    endif()
    if (${TYPE} MATCHES win32gui)
        set_subsystem(${MODULE} windows)
        set_entrypoint(${MODULE} WinMainCRTStartup)
        if(IS_UNICODE)
            target_link_libraries(${MODULE} mingw_wmain)
        else()
            target_link_libraries(${MODULE} mingw_main)
        endif()
        target_link_libraries(${MODULE} mingw_common msvcsup)
    endif ()
    if (${TYPE} MATCHES win32cui)
        set_subsystem(${MODULE} console)
        set_entrypoint(${MODULE} mainCRTStartup)
        if(IS_UNICODE)
            target_link_libraries(${MODULE} mingw_wmain)
        else()
            target_link_libraries(${MODULE} mingw_main)
        endif()
        target_link_libraries(${MODULE} mingw_common msvcsup)
    endif ()
    if(${TYPE} MATCHES win32dll)
        # Need this only because mingw library is broken
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        if(DEFINED baseaddress_${MODULE})
            set_image_base(${MODULE} ${baseaddress_${MODULE}})
        else()
            message(STATUS "${MODULE} has no base address")
        endif()
        target_link_libraries(${MODULE} mingw_common mingw_dllmain msvcsup)
        add_linkerflag(${MODULE} "/DLL")
    endif()
    if(${TYPE} MATCHES win32ocx)
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
        target_link_libraries(${MODULE} mingw_common mingw_dllmain msvcsup)
        add_linkerflag(${MODULE} "/DLL")
    endif()
    if(${TYPE} MATCHES cpl)
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".cpl")
        target_link_libraries(${MODULE} mingw_common mingw_dllmain msvcsup)
        add_linkerflag(${MODULE} "/DLL")
    endif()
    if(${TYPE} MATCHES kernelmodedriver)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".sys")
        set_entrypoint(${MODULE} DriverEntry@8)
        set_subsystem(${MODULE} native)
        set_image_base(${MODULE} 0x00010000)
        add_linkerflag(${MODULE} "/DRIVER")
        add_dependencies(${MODULE} bugcodes)
        target_link_libraries(${MODULE} msvcsup)
    endif()
endmacro()

macro(set_unicode)
   add_definitions(-DUNICODE -D_UNICODE)
   set(IS_UNICODE 1)
endmacro()

macro(set_rc_compiler)
# dummy, this workaround is only needed in mingw due to lack of RC support in cmake
endmacro()

# Thanks MS for creating a stupid linker
macro(add_importlib_target _exports_file)
    get_filename_component(_name ${_exports_file} NAME_WE)
    get_target_property(_suffix ${_name} SUFFIX)
    if(${_suffix} STREQUAL "_suffix-NOTFOUND")
        get_target_property(_type ${_name} TYPE)
        if(${_type} MATCHES EXECUTABLE)
            set(_suffix ".exe")
        else()
            set(_suffix ".dll")
        endif()
    endif()

    # Generate the asm stub file and the export def file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def
        COMMAND native-spec2def --ms --kill-at -r -n=${_name}${_suffix} -d=${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def -l=${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file})

    # Assemble the stub file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj
        COMMAND ${CMAKE_ASM_COMPILER} /nologo /Cp /Fo${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj /c /Ta ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm
        DEPENDS "${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm")

    # Add neccessary importlibs for redirections
    set(_libraries "")
    foreach(_lib ${ARGN})
        list(APPEND _libraries "${CMAKE_BINARY_DIR}/importlibs/${_lib}.lib")
        list(APPEND _dependencies ${_lib})
    endforeach()

    # Build the importlib
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib
        COMMAND LINK /LIB /NOLOGO /MACHINE:X86 /DEF:${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def /OUT:${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj ${_libraries}
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def)

    # Add the importlib target
    add_custom_target(
        lib${_name}
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib)

    add_dependencies(lib${_name} asm ${_dependencies})
endmacro()

macro(add_importlibs MODULE)
    foreach(LIB ${ARGN})
        target_link_libraries(${MODULE} ${CMAKE_BINARY_DIR}/importlibs/lib${LIB}.lib)
        add_dependencies(${MODULE} lib${LIB})
    endforeach()
endmacro()

macro(spec2def _dllname _spec_file)
    get_filename_component(_file ${_spec_file} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def --ms --kill-at -n=${_dllname} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        PROPERTIES GENERATED TRUE)
endmacro()

macro(macro_mc FILE)
    set(COMMAND_MC mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc)
endmacro()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/importlibs)

#pseh workaround
set(PSEH_LIB "pseh")

