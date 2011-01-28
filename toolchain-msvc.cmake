
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

set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <DEFINES> /I${REACTOS_SOURCE_DIR}/include/psdk /I${REACTOS_BINARY_DIR}/include/psdk /I${REACTOS_SOURCE_DIR}/include /I${REACTOS_SOURCE_DIR}/include/reactos /I${REACTOS_BINARY_DIR}/include/reactos /I${REACTOS_SOURCE_DIR}/include/reactos/wine /I${REACTOS_SOURCE_DIR}/include/crt /I${REACTOS_SOURCE_DIR}/include/crt/mingw32 /fo <OBJECT> <SOURCE>")

SET(CMAKE_ASM_COMPILE_OBJECT
    "<CMAKE_C_COMPILER> /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm <FLAGS> <DEFINES> /D__ASM__ /D_USE_ML /EP /c <SOURCE> > <OBJECT>.tmp"
    "<CMAKE_ASM_COMPILER> /nologo /Cp /Fo<OBJECT> /c /Ta <OBJECT>.tmp")

set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS /W1 /Zm1000")
set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi  /Ob0 /Od")
SET(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi /Ob0 /Od")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Prevent from using run time checking when testing the compiler
set(CMAKE_BUILD_TYPE "RelwithDebInfo" CACHE STRING "Build Type")

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE INTERNAL "")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    add_definitions(-D__i386__)
endif()
