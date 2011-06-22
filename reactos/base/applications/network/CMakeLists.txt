
add_subdirectory(arp)
if(NOT MSVC)
    add_subdirectory(dwnl) # FIXME: fix msvc build
endif()
add_subdirectory(finger)
if(NOT MSVC)
    add_subdirectory(ftp) # FIXME: fix msvc build
endif()
add_subdirectory(ipconfig)
add_subdirectory(net)
add_subdirectory(netstat)
add_subdirectory(nslookup)
add_subdirectory(ping)
add_subdirectory(route)
if(NOT MSVC)
    add_subdirectory(telnet) # FIXME: fix msvc build
endif()
add_subdirectory(tracert)
add_subdirectory(whois)
