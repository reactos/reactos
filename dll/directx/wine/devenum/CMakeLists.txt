
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x600
    -DWINVER=0x600
    -DUSE_NEW_WINE_REGISTER_RESOURCES)

spec2def(devenum.dll devenum.spec)

list(APPEND SOURCE
    createdevenum.c
    devenum_main.c
    mediacatenum.c
    parsedisplayname.c)

add_library(devenum MODULE
    ${SOURCE}
    devenum.rc
    ${CMAKE_CURRENT_BINARY_DIR}/devenum.def)

add_idl_reg_scripts(devenum registry devenum_classes.idl)

set_module_type(devenum win32dll UNICODE)
target_link_libraries(devenum wine wine_dll_canunload dmoguids strmiids uuid oldnames)
add_delay_importlibs(devenum msvfw32)
add_importlibs(devenum
    ole32 oleaut32 avicap32 winmm user32 advapi32 advapi32_vista dsound msdmo
    msvcrt kernel32 ntdll)
add_dependencies(devenum wineheaders)
add_cd_file(TARGET devenum DESTINATION reactos/system32 FOR all)
set_wine_module(devenum)
