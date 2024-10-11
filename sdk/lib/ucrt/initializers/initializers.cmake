
list(APPEND UCRT_INITIALIZERS_SOURCES
    initializers/clock_initializer.cpp
    initializers/console_input_initializer.cpp
    initializers/console_output_initializer.cpp
    initializers/fmode_initializer.cpp
    initializers/locale_initializer.cpp
    initializers/multibyte_initializer.cpp
    initializers/stdio_initializer.cpp
    initializers/timeset_initializer.cpp
    initializers/tmpfile_initializer.cpp
)

if(${ARCH} STREQUAL "i386")
    list(APPEND UCRT_INITIALIZERS_SOURCES
        initializers/i386/sse2_initializer.cpp
    )
endif()

if(${ARCH} STREQUAL "amd64" OR ${ARCH} STREQUAL "arm64")
    list(APPEND UCRT_INITIALIZERS_SOURCES
        initializers/fma3_initializer.cpp
    )
endif()
