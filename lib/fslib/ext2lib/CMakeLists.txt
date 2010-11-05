
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
add_dependencies(ext2lib psdk)
