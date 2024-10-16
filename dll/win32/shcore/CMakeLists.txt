
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(shcore.dll shcore.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/shcore_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shcore.def)

add_library(shcore SHARED ${SOURCE})
set_module_type(shcore win32dll)
target_link_libraries(shcore wine uuid)
add_importlibs(shcore shell32 gdi32 ole32 user32 advapi32 advapi32_vista shlwapi msvcrt ntdll kernel32)
add_cd_file(TARGET shcore DESTINATION reactos/system32 FOR all)
