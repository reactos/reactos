
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/win32ss/core/include)

spec2def(winsrv.dll winsrv.spec)

# The components
include(consrv.cmake)
include(usersrv.cmake)

list(APPEND SOURCE
    init.c
    winsrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winsrv.def)

add_library(winsrv SHARED ${SOURCE})
set_module_type(winsrv win32dll UNICODE ENTRYPOINT DllMain 12)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(winsrv console)
################# END  HACK #################

# Add the components
target_link_libraries(winsrv consrv usersrv)
# Add win32ksys because of NtUser...()
target_link_libraries(winsrv win32ksys libcntpr ${PSEH_LIB} ${CONSRV_TARGET_LINK_LIBS})

add_delay_importlibs(winsrv advapi32 ${CONSRV_DELAY_IMPORT_LIBS})
add_importlibs(winsrv csrsrv gdi32 user32 kernel32 ntdll ${CONSRV_IMPORT_LIBS} ${USERSRV_IMPORT_LIBS})
add_dependencies(winsrv bugcodes)
add_cd_file(TARGET winsrv DESTINATION reactos/system32 FOR all)
