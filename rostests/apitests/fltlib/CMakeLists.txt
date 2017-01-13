
list(APPEND SOURCE
    load.c
    handles.c
    instance.c
	instance_find.c
	filter_find.c
	volume_find.c
	info.c
	comms.c
    testlist.c)

add_executable(fltlib_apitest ${SOURCE})
set_module_type(fltlib_apitest win32cui)
add_importlibs(fltlib_apitest user32 msvcrt kernel32)
add_rostests_file(TARGET fltlib_apitest)
