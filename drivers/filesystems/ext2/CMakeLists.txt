
include_directories(inc)

list(APPEND SOURCE
    src/volinfo.c
    src/create.c
    src/write.c
    src/metadata.c
    src/fastio.c
    src/dircntrl.c
    src/DiskIO.c
    src/fsctrl.c
    src/devcntrl.c
    src/flush.c
    src/ext2init.c
    src/io.c
    src/close.c
    src/fileinfo.c
    src/read.c
    src/cleanup.c
    src/misc.c
    src/shutdown.c
    inc/ext2fsd.h)

add_library(ext2fs SHARED ${SOURCE})

if((NOT MSVC) AND (NOT CMAKE_C_COMPILER_ID STREQUAL "Clang"))
    add_target_compile_flags(ext2fs "-Wno-unused-but-set-variable")
endif()

target_link_libraries(ext2fs ${PSEH_LIB})
add_pch(ext2fs inc/ext2fsd.h SOURCE)
set_module_type(ext2fs kernelmodedriver)
add_importlibs(ext2fs ntoskrnl hal)
add_cd_file(TARGET ext2fs DESTINATION reactos/system32/drivers NO_CAB FOR all)
