
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(msdmo.dll msdmo.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dmoreg.c
    dmort.c)

add_library(msdmo MODULE
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msdmo_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msdmo.def)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(msdmo PRIVATE -Wno-array-parameter)
endif()

set_module_type(msdmo win32dll)
target_link_libraries(msdmo wine dmoguids strmiids uuid)
add_importlibs(msdmo user32 advapi32 ole32 msvcrt kernel32 ntdll)
add_dependencies(msdmo wineheaders)
add_cd_file(TARGET msdmo DESTINATION reactos/system32 FOR all)
set_wine_module(msdmo)
