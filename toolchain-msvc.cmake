
# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)
set(CMAKE_RC_COMPILER rc)
if(${ARCH} MATCHES amd64)
    set(CMAKE_ASM_COMPILER ml64)
else()
    set(CMAKE_ASM_COMPILER ml)
endif()
set(CMAKE_ASM_COMPILER_ID "VISUAL")

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE INTERNAL "")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    add_definitions(-D__i386__)
endif()
