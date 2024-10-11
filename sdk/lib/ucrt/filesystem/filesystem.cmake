
list(APPEND UCRT_FILESYSTEM_SOURCES
    filesystem/access.cpp
    filesystem/chmod.cpp
    filesystem/findfile.cpp
    filesystem/fullpath.cpp
    filesystem/makepath.cpp
    filesystem/mkdir.cpp
    filesystem/rename.cpp
    filesystem/rmdir.cpp
    filesystem/splitpath.cpp
    filesystem/stat.cpp
    filesystem/unlink.cpp
    filesystem/waccess.cpp
    filesystem/wchmod.cpp
    filesystem/wmkdir.cpp
    filesystem/wrename.cpp
    filesystem/wrmdir.cpp
    filesystem/wunlink.cpp
)

if(MSVC)
    # Disable warning C4838: conversion from 'int' to 'size_t' requires a narrowing conversion
    set_source_files_properties(filesystem/splitpath.cpp PROPERTIES COMPILE_FLAGS "/wd4838")
endif()
