
add_executable(certmgr
    certmgr.cpp
    certmgr.h
    certmgr.rc
    resource.h

    CCert.hpp
    CStore.hpp
    CStoreList.cpp
    CStoreList.hpp
    CSplitter.hpp
    CSortableHeader.hpp
)
set_module_type(certmgr win32gui UNICODE)
target_link_libraries(certmgr uuid cpprt atl_classes)
add_importlibs(certmgr cryptui crypt32 ole32 user32 msvcrt kernel32 ntdll)
add_dependencies(certmgr psdk)
add_cd_file(TARGET certmgr DESTINATION reactos/system32 FOR all)
