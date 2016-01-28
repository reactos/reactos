
add_subdirectory(cmlib)
add_subdirectory(inflib)

if(CMAKE_CROSSCOMPILING)

add_subdirectory(3rdparty)
add_subdirectory(atl)
add_subdirectory(cportlib)
add_subdirectory(cryptlib)
#add_subdirectory(dnslib) Nothing links to this lib.
add_subdirectory(drivers)
add_subdirectory(epsapi)
add_subdirectory(fast486)
add_subdirectory(fslib)
if (STACK_PROTECTOR)
    add_subdirectory(gcc_ssp)
endif()
add_subdirectory(lsalib)
add_subdirectory(ppcmmu)
add_subdirectory(pseh)
add_subdirectory(recyclebin)

if(KDBG)
    add_subdirectory(rossym)
endif()

add_subdirectory(rtl)
add_subdirectory(sdk)
add_subdirectory(smlib)
add_subdirectory(tdilib)

else()

add_subdirectory(3rdparty/zlib)

endif()
