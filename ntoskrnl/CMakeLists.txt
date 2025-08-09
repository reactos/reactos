
PROJECT(NTOS)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Enable this again. CORE-17637
    add_compile_options(-Wunused-result)
endif()

# AMD64 kernel requires special compiler flags for proper operation
if(ARCH STREQUAL "amd64")
    # Use large code model instead of kernel to avoid PIC incompatibility
    # Large model allows code/data anywhere in 64-bit address space
    # This is more compatible with various GCC builds
    add_compile_options(-mcmodel=large)
    
    # Disable red zone for interrupt safety (required for AMD64 kernels)
    add_compile_options(-mno-red-zone)
    
    # Re-enable optimizations now that we have proper code model
    # -O2 should work correctly with -mcmodel=large
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
endif()

include(ntos.cmake)

set(NTOSKRNL_SOURCE ${SOURCE})
set(NTOSKRNL_ASM_SOURCE ${ASM_SOURCE})

set(NTKRNLMP_SOURCE ${SOURCE})
set(NTKRNLMP_ASM_SOURCE ${ASM_SOURCE})

spec2def(ntoskrnl.exe ntoskrnl.spec ADD_IMPORTLIB)

# Embed RTC libs
if (STACK_PROTECTOR)
    target_sources(libntoskrnl PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

add_asm_files(ntoskrnl_asm ${NTOSKRNL_ASM_SOURCE})

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Clang optimises strcmp calls to memcmp.
    target_sources(libntoskrnl PRIVATE $<TARGET_OBJECTS:memcmp>)
endif()

if((CMAKE_C_COMPILER_ID STREQUAL "Clang") OR (CMAKE_C_COMPILER_ID STREQUAL "GNU"))
    # Enable xsave feature
    add_compile_options(-mxsave)
endif()

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_executable(ntoskrnl
    ${ntoskrnl_asm}
    ${NTOSKRNL_SOURCE}
    ${PCH_SKIP_SOURCE}
    ntoskrnl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntoskrnl.def)
# Set ImageBase to match where FreeLdr loads the kernel (0xFFFFF80000200000)
# This ensures relocations are not needed and globals work correctly
if(ARCH STREQUAL "amd64")
    set_module_type(ntoskrnl kernel IMAGEBASE 0xFFFFF80000200000)
else()
    set_module_type(ntoskrnl kernel)
endif()

source_group(TREE ${REACTOS_SOURCE_DIR}/ntoskrnl PREFIX "Source Files" FILES ${NTOSKRNL_SOURCE})

target_link_libraries(ntoskrnl cportlib csq ${PSEH_LIB} arbiter cmlib ntlsalib rtl ${ROSSYM_LIB} libcntpr wdmguid poguid ioevent)

if(STACK_PROTECTOR)
    target_sources(ntoskrnl PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

add_importlibs(ntoskrnl hal kdcom bootvid)
add_pch(ntoskrnl ${REACTOS_SOURCE_DIR}/ntoskrnl/include/ntoskrnl.h "${PCH_SKIP_SOURCE}")
add_dependencies(ntoskrnl psdk asm)
add_cd_file(TARGET ntoskrnl DESTINATION reactos/system32 NO_CAB FOR all)

add_subdirectory(ntkrnlmp)

add_asm_files(ntdllsys_asm ntdll.S)
add_library(ntdllsys ${ntdllsys_asm})
set_target_properties(ntdllsys PROPERTIES LINKER_LANGUAGE "C")
add_dependencies(ntdllsys asm)
