add_executable(ntfs_test ntfs.c)
set_module_type(ntfs_test win32cui)
add_importlibs(ntfs_test msvcrt kernel32)
