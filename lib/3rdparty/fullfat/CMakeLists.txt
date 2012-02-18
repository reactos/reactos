
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/fullfat)

add_definitions(-D__NTDRIVER__)

list(APPEND SOURCE
    ff_blk.c
    ff_crc.c
    ff_dir.c
    ff_error.c
    ff_fat.c
    ff_file.c
    ff_hash.c
    ff_ioman.c
    ff_memory.c
    ff_safety.c
    ff_string.c
    ff_time.c)

add_library(fullfat ${SOURCE})
allow_warnings(fullfat)
add_dependencies(fullfat bugcodes)
