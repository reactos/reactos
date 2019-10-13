
PROJECT(mmc)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)
add_definitions(-D_ATL_NO_EXCEPTIONS)

list(APPEND CPP_SOURCE
    mmc.cpp
    # This is here so that they show up in visual studio:
    CAddDialog.h
    CConsoleWnd.h
    CMainWnd.h
    CSnapin.h
    CImageList.h
    precomp.h
    resource.h)

add_rc_deps(mmc.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/mmc.ico)
add_executable(mmc_new ${CPP_SOURCE} mmc.rc)
set_module_type(mmc_new win32gui UNICODE)
target_link_libraries(mmc_new uuid mmc_uuids)
add_importlibs(mmc_new ole32 shlwapi advapi32 comdlg32 comctl32 user32 msvcrt kernel32 ntdll)
add_pch(mmc_new precomp.h CPP_SOURCE)
add_dependencies(mmc_new psdk)
add_cd_file(TARGET mmc_new DESTINATION reactos/system32 FOR all)
