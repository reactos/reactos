
list(APPEND UCRT_VCRUNTIME_SOURCES
    vcruntime/__report_gsfailure.c
    vcruntime/__report_rangecheckfailure.c
    vcruntime/__security_init_cookie.c
)

if(${ARCH} STREQUAL "i386")
    list(APPEND UCRT_VCRUNTIME_SOURCES
        vcruntime/i386/__security_check_cookie.s
    )
elseif(${ARCH} STREQUAL "amd64")
    list(APPEND UCRT_VCRUNTIME_SOURCES
        vcruntime/amd64/__security_check_cookie.s
    )
endif()
