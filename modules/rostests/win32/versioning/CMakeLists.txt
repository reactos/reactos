remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_executable(versioning_xp versioning.c versioning.rc)
set_module_type(versioning_xp win32cui UNICODE)
add_importlibs(versioning_xp user32 msvcrt kernel32)
add_rostests_file(TARGET versioning_xp)

add_executable(versioning_win10 versioning.c versioning.rc)
target_compile_definitions(versioning_win10 PRIVATE WIN10_VERSION)
set_module_type(versioning_win10 win32cui UNICODE)
add_importlibs(versioning_win10 user32 msvcrt kernel32)
add_rostests_file(TARGET versioning_win10)
