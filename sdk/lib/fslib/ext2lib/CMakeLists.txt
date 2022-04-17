
list(APPEND SOURCE
    Badblock.c
    Bitmap.c
    Disk.c
    Group.c
    Inode.c
    Memory.c
    Mke2fs.c
    Super.c
    Uuid.c
    Mke2fs.h)

add_library(ext2lib ${SOURCE})
add_pch(ext2lib Mke2fs.h SOURCE)
add_dependencies(ext2lib psdk)

if(MSVC)
    # Disable warning C4267: '=': conversion from 'size_t' to '__u8', possible loss of data
    target_compile_options(ext2lib PRIVATE /wd4267)
endif()
