
## Experimental 16-bit ASM compilation support
include(asm16.cmake)

add_subdirectory(config)
add_subdirectory(dos)
add_subdirectory(ntvdm)
add_subdirectory(pifmgr)
if(ARCH STREQUAL "i386")
add_subdirectory(samples)
endif()
add_subdirectory(vdmdbg)
add_subdirectory(wow16)
