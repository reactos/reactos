
if(NOT ARCH)
  set(ARCH i386)
endif()

# WDK support
string(REPLACE * ${ARCH} ATL_LIB_PATH $ENV{ATL_LIB_PATH})
string(REPLACE * ${ARCH} CRT_LIB_PATH $ENV{CRT_LIB_PATH})
string(REPLACE * ${ARCH} DDK_LIB_PATH $ENV{DDK_LIB_PATH})
string(REPLACE * ${ARCH} KMDF_LIB_PATH $ENV{KMDF_LIB_PATH})
string(REPLACE * ${ARCH} MFC_LIB_PATH $ENV{MFC_LIB_PATH})
string(REPLACE * ${ARCH} SDK_LIB_PATH $ENV{SDK_LIB_PATH})

link_directories(
#${ATL_LIB_PATH}
                 ${CRT_LIB_PATH}
                 ${DDK_LIB_PATH}
#                 ${IFSKIT_LIB_PATH}
#                 ${KMDF_LIB_PATH}
#                 ${MFC_LIB_PATH}
                 ${SDK_LIB_PATH})

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)
SET(CMAKE_RC_COMPILER rc)
SET(CMAKE_ASM_COMPILER ml)
SET(CMAKE_IDL_COMPILER midl)

SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <DEFINES> /I${REACTOS_SOURCE_DIR}/include/psdk /I${REACTOS_BINARY_DIR}/include/psdk /I${REACTOS_SOURCE_DIR}/include /I${REACTOS_SOURCE_DIR}/include/reactos /I${REACTOS_BINARY_DIR}/include/reactos /I${REACTOS_SOURCE_DIR}/include/reactos/wine /I${REACTOS_SOURCE_DIR}/include/crt /I${REACTOS_SOURCE_DIR}/include/crt/mingw32 /fo <OBJECT> <SOURCE>")
SET(CMAKE_IDL_COMPILE_OBJECT "<CMAKE_IDL_COMPILER> <FLAGS> <DEFINES> /win32 /h <OBJECT> <SOURCE>")

set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS /W1 /Zm1000")
set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi  /Ob0 /Od")
SET(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi /Ob0 /Od")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Prevent from using run time checking when testing the compiler
set(CMAKE_BUILD_TYPE "RelwithDebInfo" CACHE STRING "Build Type")

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE INTERNAL "")
