
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(odbccp32.dll odbccp32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    odbccp32.c
    ${CMAKE_CURRENT_BINARY_DIR}/odbccp32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/odbccp32.def)

add_library(odbccp32 SHARED ${SOURCE})
set_module_type(odbccp32 win32dll)
target_link_libraries(odbccp32 uuid wine)
add_importlibs(odbccp32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET odbccp32 DESTINATION reactos/system32 FOR all)
