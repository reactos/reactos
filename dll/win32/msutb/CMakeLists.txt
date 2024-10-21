
spec2def(msutb.dll msutb.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    msutb.cpp)

#file(GLOB msutb_rc_deps res/*.*)
#add_rc_deps(msutb.rc ${msutb_rc_deps})

add_library(msutb MODULE
    ${SOURCE}
    msutb.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msutb.def)
set_module_type(msutb win32dll UNICODE)
add_dependencies(msutb msctf psdk)
target_link_libraries(msutb cpprt wine uuid atl_classes cicero)
add_importlibs(msutb user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_delay_importlibs(msutb uxtheme imm32 shlwapi comctl32 msctf ole32 oleacc oleaut32 shell32)
add_cd_file(TARGET msutb DESTINATION reactos/system32 FOR all)
