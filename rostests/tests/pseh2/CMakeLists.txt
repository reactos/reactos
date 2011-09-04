
add_definitions(-D_DLL -D__USE_CRTIMP)

add_executable(pseh2_test psehtest.c psehtest2.c)
target_link_libraries(pseh2_test wine ${PSEH_LIB})
set_module_type(pseh2_test win32cui)
add_importlibs(pseh2_test msvcrt kernel32 ntdll)
add_cd_file(TARGET pseh2_test DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    allow_warnings(pseh2_test)
endif()
