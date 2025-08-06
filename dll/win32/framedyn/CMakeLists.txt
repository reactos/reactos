
spec2def(framedyn.dll framedyn.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    chstring.cpp
    provider.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/framedyn.def)

add_library(framedyn MODULE ${SOURCE})
target_link_libraries(framedyn cpprt)
set_target_cpp_properties(framedyn WITH_EXCEPTIONS)
target_link_libraries(framedyn /home/ahmed/x-tools/x86_64-w64-mingw32/lib/gcc/x86_64-w64-mingw32/15.1.0/libgcc_eh.a)
set_module_type(framedyn win32dll UNICODE)
add_importlibs(framedyn oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET framedyn DESTINATION reactos/system32/wbem FOR all)
