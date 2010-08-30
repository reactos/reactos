cmake_minimum_required(VERSION 2.8)
project(REACTOS)

include(CMakeMacros.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES TRUE)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES TRUE)

add_definitions(-D__REACTOS__)

if(NOT CMAKE_CROSSCOMPILING)

add_definitions(-DTARGET_i386)
add_definitions(-fshort-wchar)

include_directories(${REACTOS_SOURCE_DIR}/tools/unicode)
include_directories(include)
include_directories(include/reactos)
include_directories(include/reactos/wine)

add_subdirectory(tools)
add_subdirectory(lib)

export(TARGETS widl winebuild nci FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake NAMESPACE native- )

else()

# Activate support for assembly source files
enable_language(ASM)

# Activate language support for resource files
enable_language(RC)

set(IMPORT_EXECUTABLES "${REACTOS_SOURCE_DIR}/build/ImportExecutables.cmake" CACHE FILEPATH "Host executables")
include(${IMPORT_EXECUTABLES})

# Debug Options
set(_WINKD_ 0)
set(KDBG 1)
set(DBG 1)

add_definitions(-DDBG=${DBG} -DKDBG=${KDBG})

if(DBG MATCHES 1)
add_definitions(-D_SEH_ENABLE_TRACE)
endif(DBG MATCHES 1)

if((DBG MATCHES 1) OR (KDBG MATCHES 1))
set(DBG_OR_KDBG true)
endif((DBG MATCHES 1) OR (KDBG MATCHES 1))

# Version Options
add_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502 -D_WIN32_WINDOWS=0x502 -D_SETUPAPI_VER=0x502)

# Arch Options
add_definitions(-D_M_IX86 -D_X86_ -D__i386__)

# Other
add_definitions(-DUSE_COMPILER_EXCEPTIONS)
add_definitions(-D_USE_32BIT_TIME_T)

# Compiler Core
add_definitions(-pipe -fms-extensions)

# Debugging (Note: DWARF-4 on 4.5.1 when we ship)
add_definitions(-gdwarf-2 -g2 -femit-struct-debug-detailed=none -feliminate-unused-debug-types)

# Tuning
add_definitions(-march=pentium -mtune=i686)

# Warnings
add_definitions(-Wall -Wno-char-subscripts -Wpointer-arith -Wno-multichar -Wno-error=uninitialized -Wno-unused-value  -Winvalid-pch)

# Optimizations
add_definitions(-Os -fno-strict-aliasing -ftracer -momit-leaf-frame-pointer -mpreferred-stack-boundary=2 -fno-set-stack-executable -fno-optimize-sibling-calls)

# C++ Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

include_directories(include)
include_directories(include/psdk)
include_directories(include/dxsdk)
include_directories(${REACTOS_BINARY_DIR}/include)
include_directories(${REACTOS_BINARY_DIR}/include/dxsdk)
include_directories(${REACTOS_BINARY_DIR}/include/psdk)
include_directories(${REACTOS_BINARY_DIR}/include/reactos)
include_directories(include/crt)
include_directories(include/crt/mingw32)
include_directories(include/ddk)
include_directories(include/ndk)
include_directories(include/reactos)
include_directories(include/reactos/libs)

add_subdirectory(include/psdk)
add_subdirectory(include/dxsdk)
add_subdirectory(include/reactos/idl)
add_subdirectory(include/reactos/mc)

add_subdirectory(base)
add_subdirectory(dll)
add_subdirectory(hal)
add_subdirectory(lib)
add_subdirectory(ntoskrnl)

# nci generated intermediate files

set(nci_output 
  ${REACTOS_BINARY_DIR}/ntoskrnl/include/internal/napi.h
  ${REACTOS_BINARY_DIR}/subsystems/win32/win32k/include/napi.h
  ${REACTOS_BINARY_DIR}/lib/ntdllsys/ntdll.S
  ${REACTOS_BINARY_DIR}/ntoskrnl/ex/zw.S
  ${REACTOS_BINARY_DIR}/lib/win32ksys/win32k.S
  ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys.pspec
)

# stupid nci tool can't create folders itself, se we're gonna create them for it
set(nci_folders
  ${CMAKE_CURRENT_BINARY_DIR}/dll/ntdll/def
  ${CMAKE_CURRENT_BINARY_DIR}/lib/ntdllsys/ntdll
  ${CMAKE_CURRENT_BINARY_DIR}/lib/win32ksys
  ${CMAKE_CURRENT_BINARY_DIR}/ntoskrnl/include/internal
  ${CMAKE_CURRENT_BINARY_DIR}/ntoskrnl/ex
  ${CMAKE_CURRENT_BINARY_DIR}/subsystems/win32/win32k/include
)

file(MAKE_DIRECTORY ${nci_folders})

ADD_CUSTOM_COMMAND(
  OUTPUT ${nci_output}
  COMMAND native-nci -arch ${ARCH} ${REACTOS_SOURCE_DIR}/ntoskrnl/sysfuncs.lst ${REACTOS_SOURCE_DIR}/subsystems/win32/win32k/w32ksvc.db ${nci_output}
  DEPENDS native-nci ${nci_folders}
)

ADD_CUSTOM_TARGET(ntdll_S ALL DEPENDS ${REACTOS_BINARY_DIR}/lib/ntdllsys/ntdll.S)
ADD_CUSTOM_TARGET(win32k_S ALL DEPENDS ${REACTOS_BINARY_DIR}/lib/win32ksys/win32k.S)
ADD_CUSTOM_TARGET(ntsys_pspec ALL DEPENDS ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys.pspec)
ADD_CUSTOM_TARGET(kernel_napi ALL DEPENDS ${REACTOS_BINARY_DIR}/ntoskrnl/include/internal/napi.h)
ADD_CUSTOM_TARGET(subsystem_napi ALL DEPENDS ${REACTOS_BINARY_DIR}/subsystems/win32/win32k/include/napi.h)
ADD_CUSTOM_TARGET(kernel_zw ALL DEPENDS ${REACTOS_BINARY_DIR}/ntoskrnl/ex/zw.S)

endif()

