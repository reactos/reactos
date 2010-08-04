
SET(ARCH i386)

# Choose the right MinGW prefix
if (CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
set(MINGW_PREFIX "")
else()
set(MINGW_PREFIX "mingw32-")
endif()

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER ${MINGW_PREFIX}gcc)
SET(CMAKE_CXX_COMPILER ${MINGW_PREFIX}g++)
SET(CMAKE_RC_COMPILER ${MINGW_PREFIX}windres)
SET(CMAKE_ASM_COMPILER ${MINGW_PREFIX}gcc)
SET(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -x assembler-with-cpp -o <OBJECT> <FLAGS> <DEFINES> -D__ASM__ -c <SOURCE>")
SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -I${REACTOS_SOURCE_DIR}/include/ -I${REACTOS_BINARY_DIR}/include/reactos -i <SOURCE> -o <OBJECT> <DEFINES> -DRC_INVOKED")

# Use stdcall fixups, and don't link with anything by default unless we say so
set(CMAKE_C_STANDARD_LIBRARIES "-lgcc") # We should add the environment libgcc here
SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--enable-stdcall-fixup -Wl,--kill-at -nodefaultlibs -nostdlib")

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
