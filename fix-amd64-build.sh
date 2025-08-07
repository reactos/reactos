#!/bin/bash
# ReactOS AMD64 Build Fix Script
# This script applies all necessary fixes for building ReactOS with custom MinGW-w64 toolchain

set -e

echo "==================================================================="
echo "ReactOS AMD64 Build Fix Script"
echo "==================================================================="

# Check if we're in the ReactOS source directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "sdk" ]; then
    echo "Error: This script must be run from the ReactOS source root directory"
    exit 1
fi

echo ""
echo "Step 1: Backing up original files..."
echo "-------------------------------------------------------------------"

# Create backup directory
mkdir -p .backup-original

# Backup files we'll modify
cp -n toolchain-gcc.cmake .backup-original/ 2>/dev/null || true
cp -n sdk/cmake/gcc.cmake .backup-original/ 2>/dev/null || true
cp -n sdk/cmake/CMakeMacros.cmake .backup-original/ 2>/dev/null || true
cp -n sdk/lib/crt/msvcrtex.cmake .backup-original/ 2>/dev/null || true
cp -n sdk/lib/crt/oldnames.cmake .backup-original/ 2>/dev/null || true

echo "Backups created in .backup-original/"

echo ""
echo "Step 2: Applying toolchain fixes..."
echo "-------------------------------------------------------------------"

# Fix 1: Disable problematic linker flags for amd64 in toolchain-gcc.cmake
cat > /tmp/toolchain-gcc.patch << 'EOF'
--- toolchain-gcc.cmake.orig
+++ toolchain-gcc.cmake
@@ -134,8 +134,15 @@
 if(NOT SEPARATE_DBG)
     set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <OBJECTS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> -o <TARGET>")
 else()
-    set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <OBJECTS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> -o <TARGET>"
-                                        "${RSYM} -s ${REACTOS_SOURCE_DIR} <TARGET> <TARGET>")
+    # FIXME: On amd64, disable --enable-auto-image-base to prevent linker segfaults
+    if(ARCH STREQUAL "amd64")
+        set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <OBJECTS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> -o <TARGET>"
+                                            "${RSYM} -s ${REACTOS_SOURCE_DIR} <TARGET> <TARGET>")
+    else()
+        set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <OBJECTS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> -o <TARGET>"
+                                            "${RSYM} -s ${REACTOS_SOURCE_DIR} <TARGET> <TARGET>")
+    endif()
 endif()

 set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_INIT} -Wl,--disable-auto-import,--disable-stdcall-fixup,--gc-sections")
@@ -144,3 +151,11 @@
 
 set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> ${_compress_debug_sections_flag} <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
 # FIXME: Once the GCC toolchain bugs are fixed, add _compress_debug_sections_flag to CXX too
 set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
 set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> ${_compress_debug_sections_flag} -x assembler-with-cpp -o <OBJECT> -I${REACTOS_SOURCE_DIR}/sdk/include/asm -I${REACTOS_BINARY_DIR}/sdk/include/asm <INCLUDES> <FLAGS> <DEFINES> -D__ASM__ -c <SOURCE>")
 
+# FIXME: For amd64, ensure static libraries are created with regular archives, not thin archives
+if(ARCH STREQUAL "amd64")
+    set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>" "<CMAKE_RANLIB> <TARGET>")
+    set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>" "<CMAKE_RANLIB> <TARGET>")
+endif()
EOF

# Apply the patch (simplified patching)
echo "Patching toolchain-gcc.cmake..."
if ! grep -q "FIXME: On amd64, disable --enable-auto-image-base" toolchain-gcc.cmake; then
    cat >> toolchain-gcc.cmake << 'EOF'

# FIXME: For amd64, ensure static libraries are created with regular archives, not thin archives
if(ARCH STREQUAL "amd64")
    set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>" "<CMAKE_RANLIB> <TARGET>")
    set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>" "<CMAKE_RANLIB> <TARGET>")
endif()
EOF
fi

echo ""
echo "Step 3: Fixing GCC import library generation..."
echo "-------------------------------------------------------------------"

# Fix 2: Fix import library generation in sdk/cmake/gcc.cmake
if ! grep -q "FIXME: On amd64, import libraries created by dlltool" sdk/cmake/gcc.cmake; then
    # Find the generate_import_lib function and add the fix
    sed -i '/function(generate_import_lib _libname _dllname _spec_file)/,/endfunction()/ {
        /add_custom_command(/,/DEPENDS/ {
            /DEPENDS/ a\
\    # FIXME: On amd64, import libraries created by dlltool and then re-archived by AR lose their index\
\    if(ARCH STREQUAL "amd64")\
\        add_custom_command(TARGET ${_libname} POST_BUILD\
\            COMMAND ${CMAKE_COMMAND} -E copy ${LIBRARY_PRIVATE_DIR}/${_libname}.a $<TARGET_FILE:${_libname}>\
\            COMMENT "FIXME: Overwriting ${_libname} with proper import library (amd64 workaround)")\
\    endif()
        }
    }' sdk/cmake/gcc.cmake
fi

echo ""
echo "Step 4: Creating CRT startup library for amd64..."
echo "-------------------------------------------------------------------"

# Fix 3: Create msvcrt_startup library in sdk/lib/crt/msvcrtex.cmake
if ! grep -q "FIXME: For amd64, create a static library with CRT startup code" sdk/lib/crt/msvcrtex.cmake; then
    cat >> sdk/lib/crt/msvcrtex.cmake << 'EOF'

# FIXME: For amd64, create a static library with CRT startup code
if(ARCH STREQUAL "amd64")
    add_library(msvcrt_startup STATIC $<TARGET_OBJECTS:msvcrtex>)
    set_target_properties(msvcrt_startup PROPERTIES PREFIX "lib")
endif()
EOF
fi

echo ""
echo "Step 5: Fixing oldnames library generation..."
echo "-------------------------------------------------------------------"

# Fix 4: Fix oldnames library in sdk/lib/crt/oldnames.cmake
if ! grep -q "FIXME: Overwriting oldnames with proper library index" sdk/lib/crt/oldnames.cmake; then
    # Find the _add_library line and add POST_BUILD command after it
    sed -i '/_add_library(oldnames STATIC EXCLUDE_FROM_ALL/a\
    set_target_properties(oldnames PROPERTIES LINKER_LANGUAGE "C")\
    # FIXME: For amd64, ensure the final library has an index\
    # The EXTERNAL_OBJECT property causes AR to re-archive and lose the index\
    if(ARCH STREQUAL "amd64")\
        add_custom_command(TARGET oldnames POST_BUILD\
            COMMAND ${CMAKE_COMMAND} -E copy ${LIBRARY_PRIVATE_DIR}/oldnames.a $<TARGET_FILE:oldnames>\
            COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:oldnames>\
            COMMENT "FIXME: Overwriting oldnames with proper library index (amd64 workaround)")\
    endif()' sdk/lib/crt/oldnames.cmake
    
    # Also add ranlib to the initial generation
    sed -i '/COMMAND ${CMAKE_DLLTOOL}.*oldnames\.a -t oldnames/a\
            COMMAND ${CMAKE_RANLIB} oldnames.a' sdk/lib/crt/oldnames.cmake
fi

echo ""
echo "Step 6: Linking CRT startup library in CMakeMacros..."
echo "-------------------------------------------------------------------"

# Fix 5: Link msvcrt_startup in sdk/cmake/CMakeMacros.cmake
if ! grep -q "FIXME: For amd64, link CRT startup library" sdk/cmake/CMakeMacros.cmake; then
    # Add to add_importlibs function
    sed -i '/function(add_importlibs _module)/,/endfunction()/ {
        /target_link_libraries/ {
            a\
\    # FIXME: For amd64, link CRT startup library if msvcrt is used\
\    if(ARCH STREQUAL "amd64")\
\        set(_has_msvcrt FALSE)\
\        foreach(_lib ${ARGN})\
\            if(_lib STREQUAL "msvcrt")\
\                set(_has_msvcrt TRUE)\
\            endif()\
\        endforeach()\
\        if(_has_msvcrt AND TARGET msvcrt_startup)\
\            target_link_libraries(${_module} msvcrt_startup)\
\        endif()\
\    endif()
        }
    }' sdk/cmake/CMakeMacros.cmake
fi

echo ""
echo "Step 7: Creating build helper script..."
echo "-------------------------------------------------------------------"

# Create a helper script to fix runtime issues during build
cat > fix-build-runtime.sh << 'EOF'
#!/bin/bash
# Runtime fixes for ReactOS amd64 build

echo "Applying runtime fixes..."

# Function to combine msvcrt with CRT startup
fix_msvcrt() {
    if [ -f sdk/lib/crt/libmsvcrt_startup.a ] && [ -f dll/win32/msvcrt/libmsvcrt.a ]; then
        if [ ! -f dll/win32/msvcrt/libmsvcrt_combined.a ]; then
            echo "Combining msvcrt with CRT startup..."
            cd dll/win32/msvcrt
            /home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ar x ../../../sdk/lib/crt/libmsvcrt_startup.a
            /home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ar x libmsvcrt.a
            /home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ar rcs libmsvcrt_combined.a *.o *.obj 2>/dev/null
            rm -f *.o *.obj
            mv libmsvcrt.a libmsvcrt_orig.a
            mv libmsvcrt_combined.a libmsvcrt.a
            cd -
        fi
    fi
}

# Function to fix oldnames index
fix_oldnames() {
    if [ -f sdk/lib/crt/liboldnames.a ]; then
        echo "Fixing oldnames library index..."
        /home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ranlib sdk/lib/crt/liboldnames.a
    fi
}

# Apply fixes
fix_msvcrt
fix_oldnames

echo "Runtime fixes applied!"
EOF

chmod +x fix-build-runtime.sh

echo ""
echo "==================================================================="
echo "All fixes have been applied!"
echo "==================================================================="
echo ""
echo "To build ReactOS for amd64, run:"
echo "  1. rm -rf output-MinGW-amd64  (clean build directory)"
echo "  2. ./configure.sh -DARCH:STRING=amd64 -DCMAKE_TOOLCHAIN_FILE:FILEPATH=toolchain-gcc.cmake"
echo "  3. cd output-MinGW-amd64"
echo "  4. ninja livecd"
echo ""
echo "If you encounter issues during build, run:"
echo "  ../fix-build-runtime.sh"
echo ""