
add_definitions(-D_COMDDK_)

add_library(sysaudio SHARED
    control.c
    deviface.c
    dispatcher.c
    main.c
    pin.c
    sysaudio.rc)

add_pch(sysaudio sysaudio.h)
set_module_type(sysaudio kernelmodedriver)
target_link_libraries(sysaudio libcntpr)
add_importlibs(sysaudio ntoskrnl ks hal)
add_cd_file(TARGET sysaudio DESTINATION reactos/system32/drivers FOR all)
