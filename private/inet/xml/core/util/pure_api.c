/*
 * Header file of Pure API function declarations.
 *
 * Explicitly no copyright.
 * You may recompile and redistribute these definitions as required.
 *
 * NOTE: In some situations when compiling with MFC, you should
 *       enable the setting 'Not using precompiled headers' in Visual C++
 *       to avoid a compiler diagnostic.
 *
 * NOTE: This file works through the use of deep magic.  Calls to functions
 *       in this file are replaced with calls into the OCI runtime system
 *       when an instrumented version of this program is run.
 */
#ifdef PROFILE
__declspec(dllexport) int __cdecl PurePrintf(const char *fmt, ...) { return 0; }
__declspec(dllexport) int __cdecl PurifyIsRunning(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyPrintf(const char *fmt, ...) { return 0; }
__declspec(dllexport) int __cdecl PurifyNewInuse(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyAllInuse(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyClearInuse(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyNewLeaks(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyAllLeaks(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyClearLeaks(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyAllHandlesInuse(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyNewHandlesInuse(void) { return 0; }
__declspec(dllexport) int __cdecl PurifyDescribe(void *addr) { return 0; }
__declspec(dllexport) int __cdecl PurifyWhatColors(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyAssertIsReadable(const void *addr, int size) { return 1; }
__declspec(dllexport) int __cdecl PurifyAssertIsWritable(const void *addr, int size) { return 1; }
__declspec(dllexport) int __cdecl PurifyIsReadable(const void *addr, int size) { return 1; }
__declspec(dllexport) int __cdecl PurifyIsWritable(const void *addr, int size) { return 1; }
__declspec(dllexport) int __cdecl PurifyIsInitialized(const void *addr, int size) { return 1; }
__declspec(dllexport) int __cdecl PurifyRed(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyGreen(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyYellow(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyBlue(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyMarkAsInitialized(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyMarkAsUninitialized(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyMarkForTrap(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyMarkForNoTrap(void *addr, int size) { return 0; }
__declspec(dllexport) int __cdecl PurifyHeapValidate(unsigned int hHeap, unsigned int dwFlags, const void *addr) { return 1; }
__declspec(dllexport) int __cdecl PurifySetLateDetectScanCounter(int counter) { return 0; };
__declspec(dllexport) int __cdecl PurifySetLateDetectScanInterval(int seconds) { return 0; };
__declspec(dllexport) int __cdecl QuantifyIsRunning(void) { return 0; }
__declspec(dllexport) int __cdecl QuantifyDisableRecordingData(void) { return 0; }
__declspec(dllexport) int __cdecl QuantifyStartRecordingData(void) { return 0; }
__declspec(dllexport) int __cdecl QuantifyStopRecordingData(void) { return 0; }
__declspec(dllexport) int __cdecl QuantifyClearData(void) { return 0; }
__declspec(dllexport) int __cdecl QuantifyIsRecordingData(void) { return 0; }
__declspec(dllexport) int __cdecl QuantifyAddAnnotation(char *str) { return 0; }
__declspec(dllexport) int __cdecl QuantifySaveData(void) { return 0; }
#endif