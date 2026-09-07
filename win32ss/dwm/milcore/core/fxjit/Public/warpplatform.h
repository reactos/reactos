// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/******************************Module*Header*******************************
* Module Name: warpplatform.h
*
* Header that defines interface to platform-dependent services
*

**************************************************************************/
#pragma once

#pragma warning(disable:4480)          // nonstandard extension used: specifying underlying type for enum
#pragma warning(disable:4512)          // assignment operator could not be generated
#pragma warning(disable:4201)          // nonstandard extension used : nameless struct/union
#pragma warning(disable:4615)          // unknown user warning type
#pragma warning(disable:4061)          // case not handled
#pragma warning(disable:4127)          // constant conditional

#undef C_ASSERT                        // Only defined in Winnt.h (SIMDJit doesn't include)
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

#ifndef NO_DEFAULT
#define NO_DEFAULT __assume(0)
#endif

#if !defined(__WFILE__)
    #define __t1__(x)    L##x
    #define __t2__(x)    __t1__(x)
    #define __t3__(x,y)  __t1__(#y)
    #define __WFILE__    __t2__(__FILE__)
#endif


#if DBG
#ifndef Assert
#define Assert(cond)                                                                                                      \
    {                                                                                                                     \
        __analysis_assume(cond);                                                                                          \
        if(!(cond))                                                                                                       \
        {                                                                                                                 \
            WarpPlatform::AssertMessage(L#cond, __WFILE__, __LINE__);                                               \
        }                                                                                                                 \
    }                                                                                                                     

#define AssertMsg(cond, msg)                                                                                              \
    {                                                                                                                     \
        __analysis_assume(cond);                                                                                          \
        if(!(cond))                                                                                                       \
        {                                                                                                                 \
            WarpPlatform::AssertMessage(L#msg, __WFILE__, __LINE__);                                                \
        }                                                                                                                 \
    }                                                                                                                     
#endif

#else
// Retail case

#ifndef Assert
#define Assert(x)
#endif

#ifndef AssertMsg
#define AssertMsg(cond, msg)
#endif

#endif

#define WarpError(x)  AssertMsg(0, x)
#define WarpAssert(x) Assert(x)

struct PerfMonCounters
{
    int FramesPerSecond;
    int TrianglesPerSecond;
    int LinesPerSecond;
    int PointsPerSecond;
    int JITPixelProcessorsPerSecond;
    int FlushesPerSecond;
};

extern volatile PerfMonCounters perfMonCounters;

class CProgram;

class WarpPlatform
{
public:
    //
    // Allocates memory from the heap
    // Returns NULL on failure
    //
    // NumBytes = Number of bytes to allocate
    // Alignment = The alignment of the returned pointer (set to 1 if you don't care)
    //
    static void* AllocateMemory(size_t NumBytes);

    //
    // Frees memory allocated by AllocateMemory
    // It is OK to pass NULL to this function
    //
    static void FreeMemory(void* Pointer);

    //
    // Starts a compilation session
    // This may block to ensure thread safety (because their is only ever 1 "current" program)
    //
    static void BeginCompile(CProgram* Program);

    //
    // Indicates the end of a compilation sessions
    // This must be paired with every call to BeginCompile
    //
    static void EndCompile();

    //
    // Only valid to call this during a BeginCompile/EndCompile pair
    //
    static CProgram* GetCurrentProgram();

    enum Permissions
    {
        Read,
        ReadWrite,
        ReadWriteExecute,
        Write,
    };

    typedef void* LockHandle;

    //
    // Creates a lock (critical section in user-mode, fast mutex in kernel mode)
    // returns NULL on failure
    //
    static LockHandle CreateLock();

    //
    // Frees a lock that was created with CreateLock
    //
    static void DeleteLock(LockHandle);

    //
    // Acquires a lock, does not support recursion
    //
    static void AcquireLock(LockHandle);

    //
    // Releases a previously acquired lock
    //
    static void ReleaseLock(LockHandle);

    //
    // Debug trace
    //

    static void TraceMessage(
        __in_z const unsigned short *pzTraceMessage
        );

    //
    // Debug assert trigger
    //

    static void AssertMessage(
        __in_z const unsigned short *pzCondition,
        __in_z const unsigned short *pzFile,
        unsigned nLine
        );
};

//
// Utility class to free a lock when this goes out of scope
//
class WarpPlatformAutoLock
{
public:
    WarpPlatformAutoLock(WarpPlatform::LockHandle h) : _Lock(h)
    {
        WarpPlatform::AcquireLock(h);
    }

    ~WarpPlatformAutoLock()
    {
        WarpPlatform::ReleaseLock(_Lock);
    }

private:
    WarpPlatform::LockHandle _Lock;
};


