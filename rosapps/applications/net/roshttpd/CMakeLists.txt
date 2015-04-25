
set_cpp(WITH_RUNTIME WITH_STL WITH_EXCEPTIONS)
include_directories(BEFORE include)
add_compile_flags("-D__USE_W32_SOCKETS")

list(APPEND SOURCE
    config.cpp
    error.cpp
    http.cpp
    httpd.cpp
    roshttpd.cpp
    common/list.cpp
    common/socket.cpp
    common/thread.cpp)

add_executable(roshttpd ${SOURCE} common/roshttpd.rc)
set_module_type(roshttpd win32cui)
add_importlibs(roshttpd user32 ws2_32 msvcrt kernel32)
add_cd_file(TARGET roshttpd DESTINATION reactos FOR all)
