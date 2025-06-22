
spec2def(qcap.dll qcap.spec)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)
add_definitions(-Dwcsdup=_wcsdup)
list(APPEND SOURCE
    audiorecord.c
    avico.c
    avimux.c
    capturegraph.c
    filewriter.c
    guids.c
    qcap_main.c
    smartteefilter.c
    v4l.c
    vfwcapture.c)

add_library(qcap MODULE ${SOURCE}
           ${CMAKE_CURRENT_BINARY_DIR}/qcap_stubs.c
           ${CMAKE_CURRENT_BINARY_DIR}/qcap.def
           version.rc)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(qcap PRIVATE -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable)
endif()

set_module_type(qcap win32dll)
target_link_libraries(qcap strmbase strmiids uuid wine)
add_importlibs(qcap winmm ole32 oleaut32 gdi32 advapi32 advapi32_vista msvcrt user32 kernel32_vista kernel32 ntdll)
add_delay_importlibs(qcap msvfw32)
add_cd_file(TARGET qcap DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(qcap) # CORE-5743: No CONST_VTABLE
