list(APPEND SOURCE
    create.c
    dirctl.c
    driver.c
    fsctl.c
    info.c
    misc.c
    rw.c
    exfat.h)

set(EXFAT_FATFS_SOURCE_DIR ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/fatfs)

add_library(exfat_fatfs STATIC
    ${EXFAT_FATFS_SOURCE_DIR}/ff.c
    ${EXFAT_FATFS_SOURCE_DIR}/ffunicode.c)

target_include_directories(exfat_fatfs PUBLIC ${EXFAT_FATFS_SOURCE_DIR})
target_compile_definitions(exfat_fatfs PUBLIC FF_USE_REACTOS_TYPES FF_LFN_UNICODE=1 UNICODE _UNICODE)
add_dependencies(exfat_fatfs xdk)

add_library(exfat MODULE ${SOURCE} exfat.rc)
set_module_type(exfat kernelmodedriver)
target_link_libraries(exfat exfat_fatfs ${PSEH_LIB} memcmp)
add_importlibs(exfat ntoskrnl hal)
add_pch(exfat exfat.h SOURCE)
add_cd_file(TARGET exfat DESTINATION reactos/system32/drivers FOR all)
