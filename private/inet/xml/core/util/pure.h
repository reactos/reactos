/*
 * Header file of Pure API function declarations.
 *
 * Explicitly no copyright.
 * You may recompile and redistribute these definitions as required.
 *
 * Version 1.0
 */

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

#define PURE_H_VERSION 1

//////////////////////////////
// API's Specific to Purify //
//////////////////////////////

// TRUE when Purify is running.
int __cdecl PurifyIsRunning(void)            ;
//
// Print a string to the viewer.
//
int __cdecl PurePrintf(const char *fmt, ...)        ;
int __cdecl PurifyPrintf(const char *fmt, ...)        ;
//
// Purify functions for leak and memory-in-use functionalty.
//
int __cdecl PurifyNewInuse(void)            ;
int __cdecl PurifyAllInuse(void)             ;
int __cdecl PurifyClearInuse(void)            ;
int __cdecl PurifyNewLeaks(void)            ;
int __cdecl PurifyAllLeaks(void)            ;
int __cdecl PurifyClearLeaks(void)            ;
//
// Purify functions for handle leakage.
//
int __cdecl PurifyAllHandlesInuse(void)            ;
int __cdecl PurifyNewHandlesInuse(void)            ;
//
// Functions that tell you about the state of memory.
//
int __cdecl PurifyDescribe(void *addr)            ;
int __cdecl PurifyWhatColors(void *addr, int size)     ;
//
// Functions to test the state of memory.  If the memory is not
// accessable, an error is signaled just as if there were a memory
// reference and the function returns false.
//
int __cdecl PurifyAssertIsReadable(const void *addr, int size)    ;
int __cdecl PurifyAssertIsWritable(const void *addr, int size)    ;
//
// Functions to test the state of memory.  If the memory is not
// accessable, these functions return false.  No error is signaled.
//
int __cdecl PurifyIsReadable(const void *addr, int size)    ;
int __cdecl PurifyIsWritable(const void *addr, int size)    ;
int __cdecl PurifyIsInitialized(const void *addr, int size)    ;
//
// Functions to set the state of memory.
//
void __cdecl PurifyMarkAsInitialized(void *addr, int size)    ;
void __cdecl PurifyMarkAsUninitialized(void *addr, int size)    ;
//
// Functions to do late detection of ABWs, FMWs, IPWs.
//
#define PURIFY_HEAP_CRT                     0xfffffffe
#define PURIFY_HEAP_ALL                     0xfffffffd
#define PURIFY_HEAP_BLOCKS_LIVE             0x80000000
#define PURIFY_HEAP_BLOCKS_DEFERRED_FREE     0x40000000
#define PURIFY_HEAP_BLOCKS_ALL                 (PURIFY_HEAP_BLOCKS_LIVE|PURIFY_HEAP_BLOCKS_DEFERRED_FREE)
int __cdecl PurifyHeapValidate(unsigned int hHeap, unsigned int dwFlags, const void *addr)    ;
int __cdecl PurifySetLateDetectScanCounter(int counter);
int __cdecl PurifySetLateDetectScanInterval(int seconds);


////////////////////////////////
// API's Specific to Quantify //
////////////////////////////////

// TRUE when Quantify is running.
int __cdecl QuantifyIsRunning(void)            ;

//
// Functions for controlling collection
//
int __cdecl QuantifyDisableRecordingData(void)        ;
int __cdecl QuantifyStartRecordingData(void)        ;
int __cdecl QuantifyStopRecordingData(void)        ;
int __cdecl QuantifyClearData(void)            ;
int __cdecl QuantifyIsRecordingData(void)        ;

// Add a comment to the dataset
int __cdecl QuantifyAddAnnotation(char *)        ;

// Save the current data, creating a "checkpoint" dataset
int __cdecl QuantifySaveData(void)            ;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
