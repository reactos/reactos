
add_subdirectory(cdfs)
add_subdirectory(ext2)
add_subdirectory(fastfat)
#add_subdirectory(fastfat_new)
add_subdirectory(fs_rec)
add_subdirectory(msfs)
add_subdirectory(mup)
add_subdirectory(npfs)
add_subdirectory(ntfs)
if(MSVC)
    # FIXME: Make sure this compiles with the *nix builder before enabling it
    add_subdirectory(udfs)
endif()
