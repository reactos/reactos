
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
add_definitions(
    -D__WINESRC__
    -Dstricmp=_stricmp
    -Dstrcasecmp=_stricmp
    -Dstrncasecmp=_strnicmp
    -Dmemicmp=_memicmp
    -Dtempnam=_tempnam
)

list(APPEND SOURCE
    devinst.c
    dialog.c
    diskspace.c
    install.c
    misc.c
    parser.c
    query.c
    setupcab.c
    stringtable.c
    testlist.c)

add_executable(setupapi_winetest ${SOURCE} setupapi.rc)
set_module_type(setupapi_winetest win32cui)
target_link_libraries(setupapi_winetest uuid)
add_importlibs(setupapi_winetest advapi32 setupapi user32 shell32 crypt32 wintrust cabinet ole32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET setupapi_winetest)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(setupapi_winetest PRIVATE -Wno-format-overflow -Wno-int-conversion)
elseif(MSVC)
    target_compile_options(setupapi_winetest PRIVATE /wd4047 /wd4024)
endif()

# coinst.dll
spec2def(coinst.dll coinst.spec)
add_library(coinst MODULE coinst.c ${CMAKE_CURRENT_BINARY_DIR}/coinst.def)
set_module_type(coinst win32dll UNICODE)
add_importlibs(coinst msvcrt kernel32 ntdll)
add_rostests_file(TARGET coinst)

# selfreg.dll
spec2def(selfreg.dll selfreg.spec)
add_library(selfreg MODULE selfreg.c ${CMAKE_CURRENT_BINARY_DIR}/selfreg.def)
set_module_type(selfreg win32dll UNICODE)
target_link_libraries(selfreg uuid)
add_importlibs(selfreg advapi32 ole32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET selfreg)
