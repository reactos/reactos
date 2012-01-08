
if((ARCH MATCHES i386) OR (ARCH MATCHES amd64))
    add_subdirectory(halx86)
elseif(ARCH MATCHES arm)
#    add_subdirectory(halarm)
elseif(ARCH MATCHES powerpc)
#    add_subdirectory(halppc)
endif()
