set_cpp(WITH_RUNTIME)

if(NOT MSVC)
spec2def(framedyn.dll framedyn_gcc.spec ADD_IMPORTLIB)
else()
spec2def(framedyn.dll framedyn_msvc.spec ADD_IMPORTLIB)
endif()

list(APPEND SOURCE
    main.c
    chstring.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/framedyn.def)

add_library(framedyn SHARED
    ${SOURCE})

set_module_type(framedyn win32dll UNICODE)

add_importlibs(framedyn
    msvcrt
    kernel32
    oleaut32
    ntdll)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #add_target_compile_flags(framedyn "-fexceptions -Wno-deprecated-declarations")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-fexceptions -Wno-deprecated-declarations")
endif()

add_cd_file(TARGET framedyn DESTINATION reactos/system32/wbem FOR all)
