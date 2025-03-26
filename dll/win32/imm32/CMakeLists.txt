
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D__WINESRC__)

spec2def(imm32.dll imm32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    candidate.c
    compstr.c
    ctf.c
    guideline.c
    ime.c
    imm.c
    keymsg.c
    regword.c
    softkbd.c
    utils.c
    win3.c
    ${CMAKE_CURRENT_BINARY_DIR}/imm32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/imm32.def)

list(APPEND imm32_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_100.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_101.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_102.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_103.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_104.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_105.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_106.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_107.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/1033_Bitmap_108.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_201.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_202.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_203.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_204.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_205.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_206.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_207.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_208.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/2052_Bitmap_209.bmp)

add_library(imm32 MODULE ${SOURCE} imm32.rc)
set_module_type(imm32 win32dll UNICODE ENTRYPOINT ImmDllInitialize 12)
set_source_files_properties(imm32.rc PROPERTIES OBJECT_DEPENDS "${imm32_rc_deps}")
target_link_libraries(imm32 wine win32ksys uuid)
add_importlibs(imm32 advapi32 user32 gdi32 kernel32 ntdll)
add_cd_file(TARGET imm32 DESTINATION reactos/system32 FOR all)
