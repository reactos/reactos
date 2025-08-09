# ReactOS Build Warnings - Complete TODO List

## Statistics
- **Total unique warnings:** 87
- **Files affected:** ~50+
- **Critical issues:** 15
- **High priority:** 25
- **Medium priority:** 30
- **Low priority:** 17

## Log Files
- `build_log_part_01.log` - Lines 1-2000
- `build_log_part_02.log` - Lines 2001-4000
- `build_log_part_03.log` - Lines 4001-6000
- `build_log_part_04.log` - Lines 6001-8000
- `build_log_part_05.log` - Lines 8001-10000
- `build_log_part_06.log` - Lines 10001-10634

## CRITICAL PRIORITY (Security/Crashes)

### ✅ FIXED - Buffer Overflows
- [x] `/sdk/tools/mkisofs/schilytools/mkisofs/tree.c:389` - sprintf buffer overflow
- [x] `/sdk/tools/mkisofs/schilytools/mkisofs/tree.c:2183` - sprintf buffer overflow

### ✅ FIXED - Memory Safety
- [x] `/boot/freeldr/freeldr/ui/ui.c:518` - overlapping strcpy
- [x] `/dll/opengl/mesa/eval.c:2080,2232` - dangling pointer usage

### ❌ TODO - Use After Free / Memory Issues
- [ ] `/base/applications/network/ftp/cmds.c` - free called on pointer with nonzero offset
  - **Fix:** Check pointer arithmetic before free()
- [ ] `/base/shell/cmd/misc.c` - pointer may be used after realloc
  - **Fix:** Save new pointer from realloc before use
- [ ] `/base/shell/cmd/for.c` - storing address of local variable
  - **Fix:** Allocate on heap or pass by value
- [ ] `/ntoskrnl/ke/timerobj.c` - storing address of local variable 'ListHead'
  - **Fix:** Review lifetime of linked list

### ❌ TODO - String Overflows
- [ ] `/base/services/dhcpcsvc/dhcp/adapter.c` - accessing 16 bytes in region of size 8
  - **Fix:** Check buffer sizes in AdapterFindByHardwareAddress
- [ ] `/dll/directx/wine/ddraw/device.c` - accessing 192 bytes in region of size 96
  - **Fix:** Verify compute_sphere_visibility buffer sizes
- [ ] `/dll/opengl/glu32/src/libutil/project.c` - accessing 128 bytes in region of size 32
  - **Fix:** Fix __gluMakeIdentityd matrix size
- [ ] `/dll/opengl/glu32/src/libutil/project.c` - accessing 64 bytes in region of size 16
  - **Fix:** Fix __gluMakeIdentityf matrix size

## HIGH PRIORITY (Correctness)

### ✅ FIXED - Array Issues
- [x] `/dll/opengl/mesa/clip.c:541` - array comparison

### ❌ TODO - Misleading Indentation (Logic Errors)
- [ ] `/base/shell/cmd/del.c` - if clause doesn't guard code
- [ ] `/dll/win32/oleaut32/vartype.c` - if clause doesn't guard code
- [ ] `/dll/win32/shell32/wine/classes.c` - else clause doesn't guard code
- [ ] `/dll/win32/shell32/wine/shellord.c` - if clause doesn't guard code
- [ ] `/dll/win32/wininet/internet.c` - else clause doesn't guard code
- [ ] `/drivers/bus/acpi/busmgr/utils.c` - if clause doesn't guard code
- [ ] `/drivers/storage/ide/uniata/id_init.cpp` - for clause doesn't guard code
  - **Fix for all:** Add proper braces { } around indented blocks

### ❌ TODO - Uninitialized Variables
- [ ] `/base/ctf/msctfime/profile.cpp` - m_dwFlags used uninitialized
  - **Fix:** Initialize in constructor
- [ ] `/ntoskrnl/kdbg/kdb_cli.c` - missing braces around initializer
  - **Fix:** Add proper initialization braces

### ❌ TODO - Incorrect Comparisons
- [ ] `/dll/cpl/mmsys/sounds.c` - address comparison always true
  - **Fix:** Remove redundant check or fix logic

## MEDIUM PRIORITY (Type Safety)

### ❌ TODO - Pointer/Integer Cast Issues (64-bit)
- [ ] `/dll/3rdparty/libtiff/tif_win32.c` - pointer to int cast
- [ ] `/dll/win32/shell32/wine/shellpath.c` - pointer to int cast
- [ ] `/sdk/include/psdk/basetsd.h` - pointer to int cast
  - **Fix for all:** Use UINT_PTR or SIZE_T for pointer arithmetic

### ❌ TODO - Array Parameter Mismatches
- [ ] `/sdk/lib/3rdparty/freetype/src/base/ftlcdfil.c:361` - FT_Vector* vs FT_Vector[3]
- [ ] `/dll/opengl/mesa/dlist.c:2356` - GLfloat* vs GLfloat[3]
- [ ] `/dll/opengl/mesa/vbfill.c:1094` - GLfloat* vs GLfloat[3]
- [ ] `/dll/3rdparty/libtiff/tif_luv.c` - multiple array parameter mismatches
- [ ] `/dll/opengl/glu32/src/libtess/tess.c` - GLdouble* vs GLdouble[3]
- [ ] `/dll/win32/rsaenh/sha2.c` - sha2_byte[] parameter mismatch
- [ ] `/dll/directx/wine/msdmo/dmoreg.c` - WCHAR[] parameter mismatch
  - **Fix for all:** Make declarations and definitions consistent

### ❌ TODO - Sign Comparison Issues
- [ ] `/drivers/filesystems/udfs/Include/tools.h` - signed/unsigned comparison
- [ ] `/drivers/filesystems/udfs/udf_info/dirtree.cpp` - signed/unsigned comparison
- [ ] `/drivers/filesystems/udfs/udf_info/extent.cpp` - signed/unsigned comparison
- [ ] `/drivers/filesystems/udfs/write.cpp` - signed/unsigned comparison
  - **Fix for all:** Cast to same signedness or use proper types

## LOW PRIORITY (Code Quality)

### ✅ FIXED - Redundant Checks
- [x] `/dll/win32/shell32/shelldesktop/CDirectoryWatcher.cpp` - this != NULL checks

### ❌ TODO - Virtual Function Hiding (C++)
- [ ] `/base/ctf/cicero/cicuif.h:216` - OnTimer() hidden
- [ ] `/base/ctf/msutb/msutb.cpp` - OnMenuSelect hidden
- [ ] `/dll/win32/shdocvw/mrulist.cpp:386` - _IsEqual hidden
- [ ] `/dll/win32/shell32/shell32.cpp` - CreateInstance hidden
- [ ] `/dll/win32/shlwapi/propbag.cpp` - Read/Write hidden
- [ ] `/sdk/lib/drivers/wdf/shared/inc/private/common/fxrequestcontext.hpp` - StoreAndReferenceMemory hidden
  - **Fix for all:** Use 'using' declarations or rename methods

### ❌ TODO - Unused Functions/Variables
- [ ] `/drivers/bus/acpi/acpica/include/acdebug.h` - Multiple unused ACPI functions
- [ ] `/drivers/bus/acpi/acpica/include/acpixf.h` - AcpiTracePoint, AcpiDebugPrint, etc.
- [ ] `/sdk/lib/crt/math/libm_sse2/fma3_available.c` - init_fma3 unused
- [ ] `/ntoskrnl/kdbg/kdb.c` - StackPtr set but not used
- [ ] `/ntoskrnl/mm/ARM3/session.c` - Index set but not used
  - **Fix:** Remove or use __attribute__((unused))

### ❌ TODO - Alignment Issues
- [ ] `/sdk/include/psdk/dbghelp.h` - MINIDUMP structures alignment
- [ ] `/drivers/filesystems/ext2/src/ext4/ext4_extents.c` - packed member address
  - **Fix:** Review packing pragmas or use memcpy for unaligned access

### ❌ TODO - Parser/Grammar Issues
- [ ] `/dll/win32/jscript/parser.y` - 18 reduce/reduce, 1 shift/reduce conflicts
- [ ] `/dll/win32/msi/sql.y` - 1 reduce/reduce conflict
- [ ] `/dll/win32/vbscript/parser.y` - 9 shift/reduce conflicts
- [ ] `/dll/win32/msxml3/xslpattern.y` - deprecated %pure-parser directive
- [ ] `/dll/win32/wbemprox/wql.y` - deprecated %pure-parser directive
  - **Fix:** Update grammar or use %define api.pure

### ❌ TODO - Other Issues
- [ ] `/base/applications/network/nslookup/utility.c` - strncpy no bytes copied
- [ ] `/sdk/lib/crt/string/tcslen.h` - nonnull argument compared to NULL
- [ ] `/ntoskrnl/config/i386/cmhardwr.c` - pointer signedness mismatch
- [ ] `/sdk/include/psdk/winsock2.h` - overflow in conversion
- [ ] `/sdk/include/wine/exception.h` - _JBTYPE* array parameter

## Implementation Order

### Phase 1 - Critical Security (Do First)
1. Fix use-after-free issues
2. Fix string overflow issues
3. Fix dangling pointers in timer code

### Phase 2 - Logic Errors
1. Fix misleading indentation issues
2. Fix uninitialized variables
3. Fix incorrect comparisons

### Phase 3 - Type Safety
1. Fix pointer/integer casts for 64-bit
2. Fix array parameter mismatches
3. Fix sign comparison issues

### Phase 4 - Code Quality
1. Fix virtual function hiding
2. Remove unused code
3. Fix alignment issues
4. Update parser grammars

## Testing After Fixes
```bash
# Rebuild with all warnings enabled
ninja clean
ninja -j32 2>&1 | tee new_build.log

# Check for reduced warning count
grep -c "warning:" new_build.log

# Run specific component tests
ninja test
```

## Progress Tracking
- [x] Split log file into parts (6 files)
- [x] Extract unique warnings (87 total)
- [x] Categorize by priority
- [x] Create TODO list
- [ ] Fix Critical issues (4/8 done)
- [ ] Fix High priority issues (1/15 done)
- [ ] Fix Medium priority issues (0/20 done)
- [ ] Fix Low priority issues (1/30 done)

**Total Progress: 6/87 warnings fixed (7%)**

---
*Generated: 2025-08-07*
*Next review: After Phase 1 completion*