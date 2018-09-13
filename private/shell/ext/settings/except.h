#ifndef __EXCEPTIONS_H
#define __EXCEPTIONS_H

//
// Disable error C4290: "C++ Exception Specification ignored"
// Exception specifications not supported in MS compiler.
// This can be removed once the compiler supports exception specification.
//
#pragma warning( disable : 4290 )

class Exception
{
    private:
        INT m_Reason;

    public:
        enum { Unknown };
        Exception(INT Reason = Unknown)
            : m_Reason(Reason) { }

        INT Reason(VOID)
            { return m_Reason; }
};


class MemoryException : public Exception
{
    public:
        enum { OutOfMemory, InsufficientBuffer };
        MemoryException(INT Reason = Exception::Unknown)
            : Exception(Reason) { }
};


class OutOfMemory : public MemoryException
{
    public:
        OutOfMemory(VOID)
            : MemoryException(MemoryException::OutOfMemory) { }
};


class InsufficientBuffer : public MemoryException
{
    public:
        InsufficientBuffer(VOID)
            : MemoryException(MemoryException::InsufficientBuffer) { }
};


class FileError : public Exception
{
    public:
        enum { FileRead, FileWrite };
        FileError(INT Reason = Exception::Unknown)
            : Exception(Reason) { }
};


class SyncObjError : public Exception
{
    public:
        enum { Mutex, Semaphore, CritSect, Event };
        SyncObjError(INT Reason = Exception::Unknown)
            : Exception(Reason) { }
};


class SyncObjErrorCreate : public SyncObjError
{
    public:
        SyncObjErrorCreate(INT Reason = Exception::Unknown)
            : SyncObjError(Reason) { }
};


class SyncObjErrorWait : public SyncObjError
{
    public:
        SyncObjErrorWait(INT Reason = Exception::Unknown)
            : SyncObjError(Reason) { }
};


#endif // __EXCEPTIONS_H

