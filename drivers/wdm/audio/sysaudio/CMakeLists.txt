
add_definitions(-D_COMDDK_)

list(APPEND SOURCE
    control.c
    deviface.c
    dispatcher.c
    main.c
    pin.c
    sysaudio.h)

add_library(sysaudio MODULE ${SOURCE} sysaudio.rc)
set_module_type(sysaudio kernelmodedriver)
target_link_libraries(sysaudio libcntpr)
add_importlibs(sysaudio ntoskrnl ks hal)
add_pch(sysaudio sysaudio.h SOURCE)
add_cd_file(TARGET sysaudio DESTINATION reactos/system32/drivers FOR all)
