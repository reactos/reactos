
#ifndef __ROSGLUE_H__
#define __ROSGLUE_H__

#ifndef __GNUC__
#define __attribute__(X)
// WARNING: When using __attribute__((packed)), also use psh/poppack.h !!
#endif


#define printf  VfatPrint

#define  alloc vfalloc
#define calloc vfcalloc
#define free   vffree


// extern int interactive, rw, list, verbose, test, write_immed;
// extern int atari_format;
// extern unsigned n_files;
// extern void *mem_queue;

#define FSCHECK_INTERACTIVE     0x01
#define FSCHECK_LIST_FILES      0x02
#define FSCHECK_VERBOSE         0x04
#define FSCHECK_TEST_READ       0x08
#define FSCHECK_READ_WRITE      0x10
#define FSCHECK_IMMEDIATE_WRITE 0x20

extern ULONG FsCheckFlags;
extern ULONG FsCheckTotalFiles;
extern PVOID FsCheckMemQueue;

#define interactive (FsCheckFlags & FSCHECK_INTERACTIVE)        // Enable interactive mode
#define list        (FsCheckFlags & FSCHECK_LIST_FILES)         // List path names
#define verbose     (FsCheckFlags & FSCHECK_VERBOSE)            // Verbose mode
#define test        (FsCheckFlags & FSCHECK_TEST_READ)          // Test for bad clusters
#define rw          (FsCheckFlags & FSCHECK_READ_WRITE)         // Read-Write (if TRUE) or Read-only mode (if FALSE)
#define write_immed (FsCheckFlags & FSCHECK_IMMEDIATE_WRITE)    // Write changes to disk immediately

#define atari_format FALSE

#define n_files   FsCheckTotalFiles
#define mem_queue FsCheckMemQueue


#endif /* __ROSGLUE_H__ */
