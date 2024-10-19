
add_subdirectory(atl)
add_subdirectory(cmlib)
add_subdirectory(inflib)

if(CMAKE_CROSSCOMPILING)

add_subdirectory(3rdparty)
add_subdirectory(apisets)
add_subdirectory(cicero)
add_subdirectory(comsupp)
add_subdirectory(conutils)
add_subdirectory(cportlib)
add_subdirectory(crt)
add_subdirectory(cryptlib)
add_subdirectory(cpprt)

if(MSVC)
    add_subdirectory(RunTmChk)
else()
    add_subdirectory(gcc-compat)
endif()

add_subdirectory(delayimp)
add_subdirectory(dmilib)
#add_subdirectory(dnslib) Nothing links to this lib.
add_subdirectory(drivers)
add_subdirectory(dxguid)
add_subdirectory(epsapi)
add_subdirectory(evtlib)
add_subdirectory(fast486)
add_subdirectory(fslib)

if(STACK_PROTECTOR)
    add_subdirectory(gcc_ssp)
endif()

add_subdirectory(ioevent)
add_subdirectory(lsalib)
add_subdirectory(nt)
add_subdirectory(pathcch)
add_subdirectory(pseh)

if(KDBG)
    add_subdirectory(rossym)
endif()

add_subdirectory(rtl)
add_subdirectory(scrnsave)
add_subdirectory(skiplist)
add_subdirectory(strmiids)
add_subdirectory(smlib)
add_subdirectory(tdilib)
add_subdirectory(tzlib)
add_subdirectory(udmihelp)
add_subdirectory(uuid)
add_subdirectory(wdmguid)

else()

add_subdirectory(3rdparty/zlib)

endif()
