
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(devenum.dll devenum.spec)

list(APPEND SOURCE
    createdevenum.c
    devenum_main.c
    mediacatenum.c
    parsedisplayname.c
    precomp.h)

add_library(devenum MODULE
    ${SOURCE}
    devenum.rc
    ${CMAKE_CURRENT_BINARY_DIR}/devenum.def)

set_source_files_properties(devenum.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/devenum_classes.rgs)
set_module_type(devenum win32dll UNICODE)
target_link_libraries(devenum strmiids uuid wine)
add_delay_importlibs(devenum msvfw32)
add_importlibs(devenum advapi32 advapi32_vista ole32 oleaut32 winmm user32 avicap32 msacm32 dsound msdmo msvcrt kernel32 ntdll)
add_dependencies(devenum wineheaders)
add_pch(devenum precomp.h SOURCE)
add_cd_file(TARGET devenum DESTINATION reactos/system32 FOR all)
