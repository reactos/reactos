
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/cardlib)

add_executable(sol
    solcreate.cpp
    solgame.cpp
    solitaire.cpp
    rsrc.rc)

set_unicode(sol yes)

set_module_type(sol win32gui)

target_link_libraries(sol cardlib)

add_importlibs(sol advapi32 comctl32 user32 gdi32 stdc++ gcc mingw32 msvcrt)