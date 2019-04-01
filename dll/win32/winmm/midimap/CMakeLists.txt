
spec2def(midimap.dll midimap.spec)

if(MSVC)
    # error C4133: 'function': incompatible types - from 'LPDWORD' to 'DWORD_PTR *'
    replace_compile_flags("/we4133" " ")
endif()

add_library(midimap MODULE
    midimap.c
    midimap.rc
    ${CMAKE_CURRENT_BINARY_DIR}/midimap.def)

set_module_type(midimap win32dll UNICODE)
target_link_libraries(midimap uuid wine)
add_importlibs(midimap advapi32 user32 winmm msvcrt kernel32 ntdll)
add_cd_file(TARGET midimap DESTINATION reactos/system32 FOR all)
