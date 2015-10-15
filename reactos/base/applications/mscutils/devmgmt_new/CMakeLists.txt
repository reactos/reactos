
set_cpp(WITH_RTTI WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/dll)

list(APPEND SOURCE devmgmt.cpp)

add_executable(devmgmt ${SOURCE} devmgmt.rc)
set_module_type(devmgmt win32gui UNICODE)
add_importlibs(devmgmt setupapi gdi32 user32 comctl32 advapi32 devmgr msvcrt kernel32)
add_cd_file(TARGET devmgmt DESTINATION reactos/system32 FOR all)
