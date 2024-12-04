
list(APPEND UCRT_STARTUP_SOURCES
    startup/__scrt_uninitialize_crt.cpp
    startup/abort.cpp
    startup/argv_data.cpp
    startup/argv_parsing.cpp
    startup/argv_wildcards.cpp
    startup/argv_winmain.cpp
    startup/assert.cpp
    startup/exit.cpp
    startup/initterm.cpp
    startup/onexit.cpp
    startup/thread.cpp
)
