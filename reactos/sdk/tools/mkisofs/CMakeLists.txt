
add_definitions(
    -DDUPLICATES_ONCE
    -DINS_BASE="\\".\\""
    -DSCHILY_BUILD
    -DSORTING)

include_directories(
    schilytools/include
    reactos
    ${REACTOS_BINARY_DIR}/sdk/include)

# libschily and libsiconv are licensed under CDDL whereas mkisofs is licensed under GPL.
# As such, we cannot mix the library sources with mkisofs sources and have to build them seperately.
# Later on, they may still be linked together though.
#
# Reference: http://opensource.stackexchange.com/questions/2094

add_library(libmdigest
    schilytools/libmdigest/sha3.c)

add_library(libschily
    schilytools/libschily/stdio/cvmod.c
    schilytools/libschily/stdio/dat.c
    schilytools/libschily/stdio/fcons.c
    schilytools/libschily/stdio/ffileread.c
    schilytools/libschily/stdio/fgetline.c
    schilytools/libschily/stdio/fileopen.c
    schilytools/libschily/stdio/filewrite.c
    schilytools/libschily/stdio/flag.c
    schilytools/libschily/stdio/niwrite.c
    schilytools/libschily/astoll.c
    schilytools/libschily/checkerr.c
    schilytools/libschily/comerr.c
    schilytools/libschily/dirent.c
    schilytools/libschily/eaccess.c
    schilytools/libschily/error.c
    schilytools/libschily/fconv.c
    schilytools/libschily/fillbytes.c
    schilytools/libschily/fnmatch.c
    schilytools/libschily/format.c
    schilytools/libschily/getargs.c
    schilytools/libschily/geterrno.c
    schilytools/libschily/getexecpath.c
    schilytools/libschily/gettimeofday.c
    schilytools/libschily/gid.c
    schilytools/libschily/jsprintf.c
    schilytools/libschily/jssnprintf.c
    schilytools/libschily/match.c
    schilytools/libschily/mem.c
    schilytools/libschily/movebytes.c
    schilytools/libschily/raisecond.c
    schilytools/libschily/saveargs.c
    schilytools/libschily/searchinpath.c
    schilytools/libschily/seterrno.c
    schilytools/libschily/streql.c
    schilytools/libschily/strlcat.c
    schilytools/libschily/strlcpy.c
    schilytools/libschily/uid.c
    schilytools/libschily/zerobytes.c)

add_library(libsiconv
    schilytools/libsiconv/sic_nls.c)
target_link_libraries(libsiconv libschily)

add_host_tool(mkisofs
    schilytools/mkisofs/boot.c
    schilytools/mkisofs/eltorito.c
    schilytools/mkisofs/hash.c
    schilytools/mkisofs/inode.c
    schilytools/mkisofs/isonum.c
    schilytools/mkisofs/joliet.c
    schilytools/mkisofs/match.c
    schilytools/mkisofs/mkisofs.c
    schilytools/mkisofs/multi.c
    schilytools/mkisofs/name.c
    schilytools/mkisofs/rock.c
    schilytools/mkisofs/stream.c
    schilytools/mkisofs/tree.c
    schilytools/mkisofs/write.c)
target_link_libraries(mkisofs libmdigest libschily libsiconv)

if(MSVC)
    # mkisofs uses K&R-style function definitions to support very old compilers.
    # MSVC complains about the resulting foo() vs. foo(void) mismatches.
    add_target_compile_flags(mkisofs "/wd4113")
else()
    # libschily implements an own printf function with support for the %r formatter.
    # Silence compilers checking for invalid formatting sequences.
    add_target_compile_flags(libschily "-Wno-format")
endif()
