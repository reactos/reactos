
list(APPEND SOURCE
    Badblock.c
    Bitmap.c
    Disk.c
    Group.c
    Inode.c
    Memory.c
    Mke2fs.c
    Super.c
    Uuid.c)
add_library(ext2lib ${SOURCE})
add_pch(ext2lib Mke2fs.h)
add_dependencies(ext2lib psdk)
allow_warnings(ext2lib)
