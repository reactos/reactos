
add_definitions(-D_COMDDK_)

add_library(sysaudio SHARED
    control.c
    deviface.c
    dispatcher.c
    main.c
    pin.c
    sysaudio.rc)

set_target_properties(sysaudio PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(sysaudio libcntpr)

add_importlibs(sysaudio ntoskrnl ks hal)
add_dependencies(sysaudio psdk bugcodes)
