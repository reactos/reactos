add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    batch.c
    testlist.c
    rsrc.rc)

add_executable(cmd_rostest ${SOURCE})
set_module_type(cmd_rostest win32cui)
add_importlibs(cmd_rostest msvcrt kernel32)
add_rostests_file(TARGET cmd_rostest)
