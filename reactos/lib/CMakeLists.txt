
add_subdirectory(cmlib)
add_subdirectory(newinflib)

if(CMAKE_CROSSCOMPILING)

add_subdirectory(3rdparty)
add_subdirectory(atl)
add_subdirectory(cportlib)
add_subdirectory(dnslib)
add_subdirectory(drivers)
add_subdirectory(epsapi)
add_subdirectory(fslib)
add_subdirectory(lsalib)
add_subdirectory(ppcmmu)
add_subdirectory(pseh)
add_subdirectory(recyclebin)
add_subdirectory(rossym)
add_subdirectory(rtl)
add_subdirectory(sdk)
add_subdirectory(smlib)
add_subdirectory(tdilib)

else()

add_subdirectory(3rdparty/zlib)

endif()
