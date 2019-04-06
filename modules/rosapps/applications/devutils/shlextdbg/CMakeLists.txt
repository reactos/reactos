
set_cpp(WITH_RUNTIME)
add_definitions(-D_ATL_NO_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

add_executable(shlextdbg shlextdbg.cpp shlextdbg.rc)

set_module_type(shlextdbg win32cui UNICODE)
target_link_libraries(shlextdbg uuid)
add_importlibs(shlextdbg ole32 comctl32 shell32 user32 msvcrt kernel32)
add_cd_file(TARGET shlextdbg DESTINATION reactos/system32 FOR all)
