
set_unicode()
set_cpp()

include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/cardlib ${CMAKE_CURRENT_SOURCE_DIR})

set_rc_compiler()

add_executable(spider
    spider.cpp
    spigame.cpp
    rsrc.rc)

target_link_libraries(spider cardlib)

set_module_type(spider win32gui)

add_importlibs(spider advapi32 comctl32 user32 gdi32 kernel32 msvcrt)

add_cd_file(TARGET spider DESTINATION reactos/system32 FOR all)
