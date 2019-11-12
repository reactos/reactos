set_cpp(WITH_RUNTIME WITH_EXCEPTIONS WITH_STL)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x603)

if(NOT MSVC)
    add_compile_flags_language("-std=c++11" "CXX")
endif()

include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs)

spec2def(shellbtrfs.dll shellbtrfs.spec)

list(APPEND SOURCE
    balance.cpp
    contextmenu.cpp
    devices.cpp
    factory.cpp
    iconoverlay.cpp
    main.cpp
    mountmgr_local.cpp
    propsheet.cpp
    reactos.cpp
    recv.cpp
    scrub.cpp
    send.cpp
    volpropsheet.cpp
    precomp.h)

add_library(shellbtrfs MODULE
    ${SOURCE}
    guid.c
    shellbtrfs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shellbtrfs.def)

file(GLOB shellbtrfs_rc_deps *.ico)
add_rc_deps(shellbtrfs.rc ${shellbtrfs_rc_deps})

set_module_type(shellbtrfs win32dll UNICODE)
target_link_libraries(shellbtrfs uuid)
add_importlibs(shellbtrfs advapi32 advapi32_vista ole32 shell32 shlwapi user32 comctl32 uxtheme setupapi comdlg32 gdi32 msvcrt kernel32_vista kernel32 ntdll)
add_pch(shellbtrfs precomp.h SOURCE)
add_cd_file(TARGET shellbtrfs DESTINATION reactos/system32 FOR all)
