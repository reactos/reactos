
set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)

if(NOT MSVC)
    spec2def(framedyn.dll framedyn_gcc.spec ADD_IMPORTLIB)
else()
    spec2def(framedyn.dll framedyn_msvc.spec ADD_IMPORTLIB)
endif()

list(APPEND SOURCE
    main.c
    chstring.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/framedyn.def)

if(MSVC)
    add_asm_files(framedyn_asm fix_exports_msvc.s)
endif()

add_library(framedyn SHARED ${SOURCE} ${framedyn_asm})
set_module_type(framedyn win32dll UNICODE)
add_importlibs(framedyn oleaut32 msvcrt kernel32 ntdll)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #add_target_compile_flags(framedyn "-Wno-deprecated-declarations")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error=deprecated-declarations")
endif()

add_cd_file(TARGET framedyn DESTINATION reactos/system32/wbem FOR all)

if(MSVC)
    add_asm_files(framedynex_asm alias_msvc.s)
else()
    add_asm_files(framedynex_asm alias_gcc.s)
endif()
add_library(framedynex ${framedynex_asm})
set_target_properties(framedynex PROPERTIES LINKER_LANGUAGE ASM)
