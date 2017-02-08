set_cpp(WITH_RUNTIME WITH_EXCEPTIONS WITH_STL)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(shellbtrfs.dll shellbtrfs.spec)

list(APPEND SOURCE
    contextmenu.cpp
    factory.cpp
    iconoverlay.cpp
    main.cpp
    propsheet.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/shellbtrfs.def)

file(GLOB shellbtrfs_rc_deps *.ico)
add_rc_deps(shellbtrfs.rc ${shellbtrfs_rc_deps})

add_library(shellbtrfs SHARED ${SOURCE} shellbtrfs.rc)
set_module_type(shellbtrfs win32dll UNICODE)
target_link_libraries(shellbtrfs uuid)
add_importlibs(shellbtrfs advapi32 advapi32_vista ole32 shell32 shlwapi user32 comctl32 uxtheme msvcrt kernel32 ntdll)
add_cd_file(TARGET shellbtrfs DESTINATION reactos/system32 FOR all)
