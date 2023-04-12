
add_executable(cleanmgr
    cleanmgr.cpp
    cleanmgr.h
    cleanmgr.rc
    resource.h
    CEmptyVolumeCacheCallBack.hpp
    CProgressDlg.hpp
    CSelectDriveDlg.cpp
    CCleanupHandler.cpp
    CCleanupHandler.hpp
    CCleanupHandlerList.cpp
    CCleanupHandlerList.hpp
)
set_module_type(cleanmgr win32gui UNICODE)
target_link_libraries(cleanmgr uuid cpprt atl_classes)
add_importlibs(cleanmgr shlwapi oleaut32 ole32 shell32 comctl32 user32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(cleanmgr psdk)
add_cd_file(TARGET cleanmgr DESTINATION reactos/system32 FOR all)
