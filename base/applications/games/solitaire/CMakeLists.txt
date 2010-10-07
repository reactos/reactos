
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/cardlib)

add_executable(sol
    solcreate.cpp
    solgame.cpp
    solitaire.cpp
    rsrc.rc)

target_link_libraries(sol stlport cardlib)

set_module_type(sol win32gui)

add_importlibs(sol advapi32 comctl32 user32 gdi32 msvcrt)
