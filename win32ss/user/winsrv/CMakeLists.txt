
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(winsrv.dll winsrv.spec)

# The components
include(consrv.cmake)
include(usersrv.cmake)

list(APPEND SOURCE
    init.c
    winsrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winsrv.def)

add_library(winsrv SHARED ${SOURCE})

# Add the components
target_link_libraries(winsrv consrv usersrv)
# Add win32ksys because of NtUser...()
target_link_libraries(winsrv win32ksys ${PSEH_LIB} ${CONSRV_TARGET_LINK_LIBS})

set_module_type(winsrv win32dll UNICODE)

add_importlibs(winsrv msvcrt kernel32 ntdll csrsrv ${CONSRV_IMPORT_LIBS})
add_delay_importlibs(winsrv user32 gdi32 advapi32 ${CONSRV_DELAY_IMPORT_LIBS})

add_dependencies(winsrv bugcodes)
add_cd_file(TARGET winsrv DESTINATION reactos/system32 FOR all)
