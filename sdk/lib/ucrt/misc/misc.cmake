
list(APPEND UCRT_MISC_SOURCES
    misc/chdir.cpp
    misc/crtmbox.cpp
    misc/drive.cpp
    misc/drivemap.cpp
    misc/drivfree.cpp
    misc/errno.cpp
    misc/exception_filter.cpp
    misc/getcwd.cpp
    misc/getpid.cpp
    misc/invalid_parameter.cpp
    misc/is_wctype.cpp
    misc/perror.cpp
    misc/resetstk.cpp
    misc/seterrm.cpp
    misc/set_error_mode.cpp
    misc/signal.cpp
    misc/slbeep.cpp
    misc/strerror.cpp
    misc/syserr.cpp
    misc/systime.cpp
    misc/terminate.cpp
    misc/wperror.cpp
    misc/_strerr.cpp
)

if(DBG)
    list(APPEND UCRT_MISC_SOURCES
        misc/dbgrpt.cpp
        misc/dbgrptt.cpp
        misc/debug_fill_threshold.cpp
    )
endif()
