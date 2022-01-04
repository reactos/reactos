Microsoft compiler-tests
========================

Introduction
------------
This repo includes selected tests from the Microsoft compiler-tests directory. 
The initial focus is on exception handling tests, both for C++EH and SEH, to make it easier to test WinEH implementations for compatibility with the platform.  The expectation is that this set of tests will grow and ultimately be added to the LLVM test-suite.  Opening this as a separate repo is intended as a stop gap as the work to get the LLVM test-suite to run clean on Windows progresses.

Supported Platforms
-------------------
The first round of tests being opened are EH, the bulk of which are SEH tests.  This is naturally Windows specific.  Additionally only the most rudimentary harness is included (runtests.cmd) due to our objective to move these tests into the LLVM harness.

Quick Start
-----------
There are two main sub directories in the compiler-tests directory.  The descriptions of what they contain are listed below.  Overtime we expect to open more tests in these directories as well as add new areas of testing.

####EH  (C++EH)
Only one test is included here now, ihateeh.cxx.  This tests object destructor semantics on Windows.  Compile the file with usual flag combinations (MSVC) and compare with the output file ihateeh.out.correct.  

####SEH
The main tests in this directory are sehframes.cpp which tests various funclet frames, and xcpt4u.c which is a large collection of SEH torture tests.  This last test is one of the main litmus tests used to verify that a compiler supports SEH suffiently to be used in the Windows kernel.  Remaining sehxxxx.c tests are particular break outs from xcpt4u.c for ease of debugging.

- Run the runtest.cmd in the seh directory to build the tests with MSVC.
- Run the clean.cmd to clean up obj/exes left after running the tests.

Next Steps
----------
More tests will follow.  If there are particular areas where there are questions please open an issue and we'll see if there are tests that can meet the need.
