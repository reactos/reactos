
set_unicode()
set_cpp()
set_rc_compiler()

include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/cardlib)

add_executable(sol
    solcreate.cpp
    solgame.cpp
    solitaire.cpp
    rsrc.rc)

target_link_libraries(sol cardlib)

set_module_type(sol win32gui)

add_importlibs(sol advapi32 comctl32 user32 gdi32 kernel32 msvcrt)

add_cd_file(TARGET sol DESTINATION reactos/system32 FOR all)
