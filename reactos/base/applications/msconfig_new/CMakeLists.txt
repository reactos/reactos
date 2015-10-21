
PROJECT(msconfig_new)

set_cpp(WITH_RUNTIME)

include_directories(
    .
    comctl32ex
    ${REACTOS_SOURCE_DIR}/include/c++)

list(APPEND C_SOURCE
    comctl32ex/listviewfuncs.c
    fileextractdialog.c
    fileutils.c
    freeldrpage.c
    generalpage.c
    msconfig.c
    # srvpage.c
    # systempage.c
    # startuppage.c
    stringutils.c
    utils.c)

list(APPEND CPP_SOURCE
    toolspage.cpp
    xmldomparser.cpp
    precomp.h)

add_rc_deps(msconfig.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/msconfig.ico)
add_executable(msconfig_new ${C_SOURCE} ${CPP_SOURCE} msconfig.rc)
add_pch(msconfig_new precomp.h CPP_SOURCE)
set_module_type(msconfig_new win32gui UNICODE)
target_link_libraries(msconfig_new comsupp)
add_importlibs(msconfig_new user32 gdi32 comctl32 comdlg32 advapi32 version ole32 oleaut32 msxml3 shell32 shlwapi msvcrt kernel32)
add_cd_file(TARGET msconfig_new DESTINATION reactos/system32 FOR all)
