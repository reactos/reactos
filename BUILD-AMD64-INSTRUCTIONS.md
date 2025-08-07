# ReactOS AMD64 Build Instructions

## Prerequisites
- Custom MinGW-w64 toolchain installed at `/home/ahmed/x-tools/x86_64-w64-mingw32/`
- CMake, Ninja, Git, Bison, Flex installed

## Critical Fixes Required

The following fixes are REQUIRED for successful AMD64 builds with custom MinGW toolchain:

### 1. Toolchain Configuration (`toolchain-gcc.cmake`)
- Disable thin archives (use regular archives)
- Always run ranlib after archive creation
- Already fixed in the source tree

### 2. Import Library Generation (`sdk/cmake/gcc.cmake`)
- Import libraries created by dlltool lose their index when re-archived
- POST_BUILD commands copy the correct library
- Already fixed in the source tree

### 3. CRT Startup Library (`sdk/lib/crt/msvcrtex.cmake`)
- Create msvcrt_startup library with CRT startup code
- Already fixed in the source tree

### 4. Oldnames Library (`sdk/lib/crt/oldnames.cmake`)
- Ensure proper index with ranlib
- Copy original library in POST_BUILD to preserve index
- Already fixed in the source tree

### 5. Runtime Fix for MSVCRT
- Combine msvcrt import library with CRT startup code
- This must be done DURING the build if linking errors occur

## Build Steps

### Step 1: Clean Build Directory
```bash
cd /home/ahmed/WorkDir/TTE/reactos
rm -rf output-MinGW-amd64
mkdir output-MinGW-amd64
cd output-MinGW-amd64
```

### Step 2: Configure Build
```bash
cmake .. -G Ninja \
    -DARCH:STRING=amd64 \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=../toolchain-gcc.cmake \
    -DCMAKE_C_COMPILER=/home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=/home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-g++
```

### Step 3: Build ReactOS
```bash
ninja livecd -j32
```

### Step 4: If CRT Startup Errors Occur (IMPORTANT)
**Note: This is likely to happen on first build due to how import libraries are generated**
If you see errors about missing mainCRTStartup, WinMainCRTStartup, or atexit:

```bash
# Option 1: Use the fix script (recommended)
../fix-msvcrt-runtime.sh
ninja livecd -j32

# Option 2: Manual fix
mkdir -p temp_fix
/home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ar x sdk/lib/crt/libmsvcrt_startup.a --output=temp_fix
/home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ar x dll/win32/msvcrt/libmsvcrt.a --output=temp_fix
/home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ar rcs dll/win32/msvcrt/libmsvcrt_fixed.a temp_fix/*.o temp_fix/*.obj
mv dll/win32/msvcrt/libmsvcrt.a dll/win32/msvcrt/libmsvcrt_original.a
mv dll/win32/msvcrt/libmsvcrt_fixed.a dll/win32/msvcrt/libmsvcrt.a
rm -rf temp_fix
ninja livecd -j32
```

### Step 5: If Oldnames Index Errors Occur
If you see "archive has no index" errors for liboldnames.a:

```bash
/home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ranlib sdk/lib/crt/liboldnames.a
ninja livecd -j32
```

## Expected Output
- Build should complete with: `livecd.iso` created in the build directory
- ISO size should be approximately 530-540 MB

## Known Issues and Solutions

1. **Linker Segmentation Faults**: Fixed by disabling --enable-auto-image-base in toolchain
2. **Archive Index Errors**: Fixed by using regular archives and running ranlib
3. **Missing CRT Startup Symbols**: Fixed by combining msvcrt with startup code
4. **Import Library Corruption**: Fixed by POST_BUILD commands in CMake

## Troubleshooting

If the build fails:
1. Check for FAILED messages in the output
2. Look for undefined reference errors
3. Verify all fixes are applied in the source tree
4. Run the runtime fixes if needed (Step 4 and 5)

## Notes
- The build process takes approximately 30-60 minutes depending on system performance
- All FIXME comments in the source indicate AMD64-specific workarounds
- These fixes are temporary workarounds for toolchain compatibility issues