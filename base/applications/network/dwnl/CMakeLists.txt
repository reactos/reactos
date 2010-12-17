add_definitions(-D_DLL -D__USE_CRTIMP)

set_unicode()

add_executable(dwnl dwnl.c)

set_module_type(dwnl win32cui)

target_link_libraries(dwnl uuid)

add_importlibs(dwnl urlmon wininet msvcrt kernel32)
if(MSVC)
add_importlibs(dwnl ntdll)
endif()

add_cab_target(dwnl 1)
