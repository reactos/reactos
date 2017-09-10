
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

add_executable(fltmc fltmc.cpp fltmc.rc)
set_module_type(fltmc win32cui UNICODE)
target_link_libraries(fltmc atlnew)
add_importlibs(fltmc fltlib msvcrt kernel32 advapi32)
add_cd_file(TARGET fltmc DESTINATION reactos/system32 FOR all)
