REM This is a simple example script to verify that the tests are working on 
REM windows. This script will be superceded by the lit/lnt as they are
REM migrated to the LLVM test-suite.   

REM set FLAGS to the desired optimization level
set FLAGS=-EHsc -O2

cl %FLAGS% ehframes.cpp
ehframes.exe > ehframes.tmp 2>&1
@if NOT ERRORLEVEL 0 echo FAIL ehframes.exe
diff ehframes.tmp ehframes.out
@if ERRORLEVEL 1 echo FAIL ehframes.exe with diffs!

cl %FLAGS% ehthrow.cxx
ehthrow.exe > ehthrow.tmp 2>&1
@if NOT ERRORLEVEL 0 echo FAIL ehthrow.exe
diff ehthrow.tmp ehthrow.amd64
@if ERRORLEVEL 1 echo FAIL ehthrow.exe with diffs!

cl %FLAGS% nesttry.cxx
nesttry.exe > nesttry.tmp 2>&1
@if NOT ERRORLEVEL 0 echo FAIL nesttry.exe
diff nesttry.tmp nesttry.out
@if ERRORLEVEL 1 echo FAIL nesttry.exe with diffs!

cl %FLAGS% noreturn.cpp
noreturn.exe
@if NOT ERRORLEVEL 0 echo FAIL noreturn.exe

cl %FLAGS% recursive_throw.cpp
recursive_throw.exe > recursive_throw.tmp 2>&1
@if NOT ERRORLEVEL 0 echo FAIL recursive_throw.exe
diff recursive_throw.tmp recursive_throw.out
@if ERRORLEVEL 1 echo FAIL recursive_throw.exe with diffs!

cl %FLAGS% rethrow1.cpp
rethrow1.exe > rethrow1.tmp 2>&1
@if NOT ERRORLEVEL 0 echo FAIL rethrow1.exe
diff rethrow1.tmp rethrow1.out
@if ERRORLEVEL 1 echo FAIL rethrow1.exe with diffs!

cl %FLAGS% rethrow4.cpp
rethrow4.exe > rethrow4.tmp 2>&1
@if NOT ERRORLEVEL 0 echo FAIL rethrow4.exe
diff rethrow4.tmp rethrow4.out
@if ERRORLEVEL 1 echo FAIL rethrow4.exe with diffs!

cl %FLAGS% rethrow5.cpp
rethrow5.exe > rethrow5.tmp 2>&1
@if NOT ERRORLEVEL 0 echo FAIL rethrow5.exe
diff rethrow5.tmp rethrow5.out
@if ERRORLEVEL 1 echo FAIL rethrow5.exe with diffs!

cl %FLAGS% rethrow_unknown.cpp
rethrow_unknown.exe
@if NOT ERRORLEVEL 0 echo FAIL rethrow_unknown.exe

cl %FLAGS% terminate.cpp
terminate.exe > terminate.tmp 2>&1
@if NOT ERRORLEVEL 0 echo FAIL terminate.exe
diff terminate.tmp terminate.out
@if ERRORLEVEL 1 echo FAIL terminate.exe with diffs!

cl %FLAGS% unreachedeh.cpp
unreachedeh.exe
@if NOT ERRORLEVEL 0 echo FAIL unreachedeh.exe

cl %FLAGS% vcatch.cpp
vcatch.exe
@if NOT ERRORLEVEL 0 echo FAIL vcatch.exe

