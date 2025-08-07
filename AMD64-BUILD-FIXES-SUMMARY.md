# ReactOS AMD64 Build Fixes Summary

## Overview
This document summarizes all the fixes required to build ReactOS for AMD64 with a custom MinGW-w64 toolchain (GCC 15.1.0, binutils 2.41.90).

## Root Causes of Build Issues

1. **Binutils 2.41.90 Bug**: Linker segfaults with certain PE flags on AMD64
2. **Import Library Generation**: Libraries created by dlltool lose their index when re-archived
3. **CRT Startup Code**: Import libraries don't contain actual startup implementations
4. **Archive Index Loss**: EXTERNAL_OBJECT property causes AR to re-archive and lose index

## Permanent Fixes Applied to Source Tree

### 1. toolchain-gcc.cmake
```cmake
# Disable problematic linker flags for AMD64
# Use regular archives instead of thin archives
# Always run ranlib after archive creation
if(ARCH STREQUAL "amd64")
    set(CMAKE_C_CREATE_STATIC_LIBRARY 
        "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>"
        "<CMAKE_RANLIB> <TARGET>")
    set(CMAKE_CXX_CREATE_STATIC_LIBRARY 
        "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>"
        "<CMAKE_RANLIB> <TARGET>")
endif()
```

### 2. sdk/cmake/gcc.cmake
```cmake
# Fix import library corruption after dlltool
if(ARCH STREQUAL "amd64")
    add_custom_command(TARGET ${_libname} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
            ${LIBRARY_PRIVATE_DIR}/${_libname}.a 
            $<TARGET_FILE:${_libname}>
        COMMENT "FIXME: Overwriting ${_libname} with proper import library")
endif()
```

### 3. sdk/lib/crt/msvcrtex.cmake
```cmake
# Create static library with CRT startup code
if(ARCH STREQUAL "amd64")
    add_library(msvcrt_startup STATIC $<TARGET_OBJECTS:msvcrtex>)
    set_target_properties(msvcrt_startup PROPERTIES PREFIX "lib")
endif()
```

### 4. sdk/lib/crt/oldnames.cmake
```cmake
# Fix oldnames library index
if(ARCH STREQUAL "amd64")
    add_custom_command(TARGET oldnames POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
            ${LIBRARY_PRIVATE_DIR}/oldnames.a 
            $<TARGET_FILE:oldnames>
        COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:oldnames>
        COMMENT "FIXME: Overwriting oldnames with proper library index")
endif()
```

### 5. dll/win32/msvcrt/CMakeLists.txt
```cmake
# Combine msvcrt with CRT startup code
if(ARCH STREQUAL "amd64")
    add_dependencies(libmsvcrt msvcrt_startup)
    add_custom_command(TARGET libmsvcrt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory 
            ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_combine_tmp
        COMMAND ${CMAKE_AR} x $<TARGET_FILE:msvcrt_startup> 
            --output=${CMAKE_CURRENT_BINARY_DIR}/msvcrt_combine_tmp
        COMMAND ${CMAKE_AR} x $<TARGET_FILE:libmsvcrt> 
            --output=${CMAKE_CURRENT_BINARY_DIR}/msvcrt_combine_tmp
        COMMAND ${CMAKE_AR} rcs 
            ${CMAKE_CURRENT_BINARY_DIR}/libmsvcrt_combined.a 
            ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_combine_tmp/*.o 
            ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_combine_tmp/*.obj
        COMMAND ${CMAKE_COMMAND} -E copy 
            ${CMAKE_CURRENT_BINARY_DIR}/libmsvcrt_combined.a 
            $<TARGET_FILE:libmsvcrt>
        COMMAND ${CMAKE_COMMAND} -E remove_directory 
            ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_combine_tmp
        COMMENT "FIXME: Combining msvcrt with CRT startup for amd64")
endif()
```

## Runtime Fix (If Needed)

If CRT startup errors occur during build:

```bash
# Use the provided script
../fix-msvcrt-runtime.sh
ninja livecd -j32

# Or manually:
mkdir -p temp_fix
AR=/home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ar
$AR x sdk/lib/crt/libmsvcrt_startup.a --output=temp_fix
$AR x dll/win32/msvcrt/libmsvcrt.a --output=temp_fix
$AR rcs dll/win32/msvcrt/libmsvcrt_fixed.a temp_fix/*.o temp_fix/*.obj
mv dll/win32/msvcrt/libmsvcrt.a dll/win32/msvcrt/libmsvcrt_original.a
mv dll/win32/msvcrt/libmsvcrt_fixed.a dll/win32/msvcrt/libmsvcrt.a
rm -rf temp_fix
```

## Build Process

1. Configure with custom toolchain
2. Start build with `ninja livecd -j32`
3. If CRT errors occur, apply runtime fix
4. Resume build
5. ISO should be created (~530-540 MB)

## Why These Fixes Work

1. **Regular Archives**: Thin archives (crT) lose symbols during re-archiving
2. **Ranlib**: Ensures archive index is always present
3. **POST_BUILD Copy**: Preserves original library before AR corrupts it
4. **CRT Startup Library**: Provides actual implementations, not just import stubs
5. **Combined msvcrt**: Merges import stubs with startup implementations

## Testing

All fixes have been tested with:
- MinGW-w64 GCC 15.1.0
- Binutils 2.41.90
- ReactOS master branch
- AMD64 architecture

## Known Limitations

- These are workarounds for toolchain issues
- Proper fix would require changes to binutils/dlltool
- The msvcrt combination may need to be done manually on first build
- All fixes are marked with FIXME comments for future reference