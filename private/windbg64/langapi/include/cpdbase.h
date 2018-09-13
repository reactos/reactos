//------------------------------------------------------------------------------
// CPDBASE.H
//
// Definitions of classes and interfaces used for the "Class Path Database"
//------------------------------------------------------------------------------
#ifndef _CPDBASE_INCLUDED
#define _CPDBASE_INCLUDED

#include "clstypes.h"
#include "cpdguid.h"

interface   ICPDatabase;
interface   IPackage;
interface   IClass;
interface   IInputStream;

typedef ICPDatabase     *LPCPDATABASE;
typedef IPackage        *LPPACKAGE;
typedef IClass          *LPCLASS;
typedef IInputStream    *LPINPUTSTREAM;


//------------------------------------------------------------------------------
// ICPDatabase
//
// This interface is the "entry point" to the class database.  It is used to
// configure the classpath, and to find specific classes/packages (or get the
// entire list of them) in the "root" of the CLASSPATH.  It also holds central
// functionality such as releasing arrays/memory allocated by this and other
// interfaces dealing with the class path database.
//------------------------------------------------------------------------------
#undef INTERFACE
#define INTERFACE ICPDatabase

DECLARE_INTERFACE_(ICPDatabase, IUnknown)
{
    // IUnknown methods
    
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // ICPDatabase methods
    
    // CLASSPATH management (note:  GetClassPath returns allocated data -- use FreeMemory to release)
    STDMETHOD(SetClassPath)(THIS_ PCSTR pszClassPath) PURE;
    STDMETHOD(AppendClassPath)(THIS_ PCSTR pszAppendPath) PURE;
    STDMETHOD(PrependClassPath)(THIS_ PCSTR pszPrependPath) PURE;
    STDMETHOD(RemoveClassPath)(THIS_ PCSTR pszRemovePath) PURE;
    STDMETHOD(GetClassPath)(THIS_ PSTR *ppszClassPath) PURE;
    
    // Archive locking means speed improvement.  It also means any .ZIP or other
    // single-file archives that are classpath roots will remain open throughout
    // use of the database, or until unlocked
    STDMETHOD_(VOID, LockArchives)(THIS) PURE;
    STDMETHOD_(VOID, UnlockArchives)(THIS) PURE;
    
    // Find specific packages or classes.  Note that the IPackage or IClass pointers returned
    // here should be Release()'d
    STDMETHOD(FindPackage)(THIS_ PCSTR pszPackageName, LPPACKAGE *ppPackage) PURE;
    STDMETHOD(FindClass)(THIS_ PCSTR pszClassName, LPCLASS *ppClass) PURE;
    
    // Given a file name, create a class interface
    STDMETHOD(GetClass)(THIS_ PCSTR pszClassFileName, LPCLASS *ppClass) PURE;
    
    // Package/class array access.  Release these arrays with appropriate Release*Array()
    // members below -- do NOT call Release() on each element!
    STDMETHOD(GetPackageArray)(THIS_ LPPACKAGE **pppPackages, LPINT piCount) PURE;
    STDMETHOD(GetClassArray)(THIS_ LPCLASS **pppClasses, LPINT piCount) PURE;
    
    // Find all occurances of a specific class name, searching all packages.  Note that
    // an array is returned, which must also be freed using ReleaseClassArray().
    STDMETHOD(GetClassArray)(THIS_ PCSTR pszClassName, LPCLASS **pppClasses, LPINT piCount) PURE;
    
    // Array release mechanism.  Note:  These release functions should be used to
    // release arrays obtained thru IPackage::GetPackageArray/GetClassArray as well.
    // Elements of these arrays should NOT be Release()'d individually unless they
    // are AddRef()'d first (for individual extension of their lifetimes)
    STDMETHOD(ReleasePackageArray)(THIS_ LPPACKAGE *ppPackages) PURE;
    STDMETHOD(ReleaseClassArray)(THIS_ LPCLASS *ppClasses) PURE;
    
    // Allocated memory release mechanism -- note that this should be used to
    // free memory returned by members of this interface as well as IPackage and
    // IClass that are noted as returning allocated memory
    STDMETHOD(FreeMemory)(THIS_ PVOID pData) PURE;
};


//------------------------------------------------------------------------------
// IPackage
//
// This interface is used to represent a package on the CLASSPATH, and its
// primary purpose is to provide subpackage and class finding/iteration.
//------------------------------------------------------------------------------
#undef INTERFACE
#define INTERFACE IPackage

DECLARE_INTERFACE_(IPackage, IUnknown)
{
    // IUnknown methods
    
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IPackage methods

    // Find specific packages or classes.  Note that the IPackage or IClass pointers returned
    // here should be Release()'d
    STDMETHOD(FindPackage)(THIS_ PCSTR pszPackageName, LPPACKAGE *ppPackage) PURE;
    STDMETHOD(FindClass)(THIS_ PCSTR pszClassName, LPCLASS *ppClass) PURE;

    // Package/class array access.  Release these arrays with appropriate Release*Array()
    // members in ICPDatabase -- do NOT call Release() on each element!
    STDMETHOD(GetPackageArray)(THIS_ LPPACKAGE **pppPackages, LPINT piCount) PURE;
    STDMETHOD(GetClassArray)(THIS_ LPCLASS **pppClasses, LPINT piCount) PURE;
    
    // Package name
    STDMETHOD_(PCSTR, GetName)(THIS) PURE;

    // Full name (i.e. if this is package lang, full name is "java.lang")
    // (allocated -- use ICPDatabase::FreeMemory to release)
    STDMETHOD(GetFullName)(THIS_ PSTR *pszFullName) PURE;
};

//------------------------------------------------------------------------------
// IClass
//
// This interface represents a class found on the CLASSPATH.  Provides access to
// all information about the class.
//------------------------------------------------------------------------------
#undef INTERFACE
#define INTERFACE IClass

DECLARE_INTERFACE_(IClass, IUnknown)
{
    // IUnknown methods
    
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IClass methods

    // Class name
    STDMETHOD_(PCSTR, GetName)(THIS) PURE;

    // Qualified (dotted) name.  Includes package name.
    // (allocated -- use ICPDatabase::FreeMemory to release)
    STDMETHOD(GetQualifiedName)(THIS_ PSTR *pszQualifiedName) PURE;

    // File name.  If this class is represented by a physical file on disk
    // (as opposed to one found in an archive or elsewhere) the full path name
    // of that file can be obtained here.  E_FAIL is returned if not.
    // (allocated -- use ICPDatabase::FreeMemory to release)
    STDMETHOD(GetFileName)(THIS_ PSTR *pszFileName) PURE;

    // Source file name.  If this class is represented by a physical file on
    // disk, the full path name of its source file can be obtained here.  E_FAIL
    // is returned if the class doesn't have a source that is represented by
    // a single disk file (i.e. the source might be in an archive as well, in
    // which case a full path to it doesn't make sense).  Note:  The class
    // information itself is opened and the attributes are searched to determine
    // the source file name.  In the interesting case where only the .java file
    // is found along the class path (.class file doesn't exist), the full path
    // to that .java file is returned, even though it *might not* necessarily
    // generate an output with the same name.  Note also that the file does not
    // have to exist for this to succeed.
    // (allocated -- use ICPDatabase::FreeMemory to release)
    STDMETHOD(GetSourceFileName)(THIS_ PSTR *pszSourceName) PURE;

    // "Moniker" -- a somewhat-human-readable name that describes where this
    // class originates.  Disk files are just the absolute file name (same as
    // GetFileName), archive files might be something that looks like
    // "archive.zip(dir1\dir2\file.ext)", etc...
    // (allocated -- use ICPDatabase::FreeMemory to release)
    STDMETHOD(GetMoniker)(THIS_ PSTR *pszMoniker) PURE;

    // Ditto for source
    // (allocated -- use ICPDatabase::FreeMemory to release)
    STDMETHOD(GetSourceMoniker)(THIS_ PSTR *pszMoniker) PURE;

    // Dependency checking between source and output.  This method returns TRUE
    // only if the class's source file is available as a single disk file, and
    // either:
    //  1) The .class file doesn't exist, or
    //  2) The .class file is older than the .java file
    // Returns FALSE in all other cases.
    STDMETHOD_(BOOL, IsOutOfDate)(THIS) PURE;

    // Direct class data access.  If this class is represented by a stream of
    // bytes in the class file format, an input stream interface is returned
    // for direct access to that stream.  E_FAIL is returned if no such file
    // exists.
    STDMETHOD(GetClassInputStream)(THIS_ LPINPUTSTREAM *ppStream) PURE;

    // Source file stream access.  Note that this may succeed when
    // GetSourceFileName() may not, since the source may be found in an archive
    STDMETHOD(GetSourceInputStream)(THIS_ LPINPUTSTREAM *ppStream) PURE;

    // Class data access.  Availability of class data depends on "completeness"
    // of the Open() call.  If fFullOpen is TRUE, all data is available; if
    // FALSE, you can get the version info and access flags, and queries for
    // other data will return 0/NULL.
    //
    // NOTE:  Everything returned from these methods is maintained by the class,
    // and does NOT need to be released or freed.  Happens automatically on Close().
    STDMETHOD(Open)(THIS_ BOOL fFullOpen = TRUE) PURE;
    STDMETHOD_(VOID, Close)(THIS) PURE;
    STDMETHOD_(U2, GetMajorVersion)(THIS) PURE;
    STDMETHOD_(U2, GetMinorVersion)(THIS) PURE;
    STDMETHOD_(U2, GetCPCount)(THIS) PURE;
    STDMETHOD_(LPCPOOLINFO, GetCPArray)(THIS) PURE;
    STDMETHOD_(U2, GetAccessFlags)(THIS) PURE;
    STDMETHOD_(U2, GetThisClass)(THIS) PURE;
    STDMETHOD_(U2, GetSuperClass)(THIS) PURE;
    STDMETHOD_(U2, GetInterfaceCount)(THIS) PURE;
    STDMETHOD_(U2 *, GetInterfaceArray)(THIS) PURE;
    STDMETHOD_(U2, GetFieldCount)(THIS) PURE;
    STDMETHOD_(LPFIELDINFO, GetFieldArray)(THIS) PURE;
    STDMETHOD_(U2, GetMethodCount)(THIS) PURE;
    STDMETHOD_(LPMETHODINFO, GetMethodArray)(THIS) PURE;
    STDMETHOD_(U2, GetAttributeCount)(THIS) PURE;
    STDMETHOD_(LPATTRINFO, GetAttributeList)(THIS) PURE;
};

//------------------------------------------------------------------------------
// IInputStream
//
// Abstraction over a file, or a section of an archive file, or anything else
// that needs to "look like" a stream of bytes w/ random access capability.
//------------------------------------------------------------------------------
#undef INTERFACE
#define INTERFACE IInputStream

typedef enum { SCP_FROMSTART = FILE_BEGIN, SCP_FROMCURRENT = FILE_CURRENT, SCP_FROMEND = FILE_END } POSRELATION;

DECLARE_INTERFACE_(IInputStream, IUnknown)
{
    // IUnknown methods
    
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    
    // IInputStream methods
    
    // Size of stream
    STDMETHOD_(LONG, GetSize)(THIS) PURE;
    
    // Current position (offset of next byte to be read)
    STDMETHOD_(LONG, GetCurrentPosition)(THIS) PURE;
    STDMETHOD(SetCurrentPosition)(THIS_ LONG iPos, POSRELATION iRel = SCP_FROMSTART) PURE;
    STDMETHOD_(BOOL, EndOfStream)(THIS) PURE;
    
    // Read from current position (piRead:  in=size of pDest, out=bytes read)
    STDMETHOD(Read)(THIS_ PVOID pDest, LONG *piRead) PURE;
    
    // Big-Endian number readers
    STDMETHOD(ReadU2)(THIS_ U2 *piValue) PURE;
    STDMETHOD(ReadU4)(THIS_ U4 *piValue) PURE;
};  


#endif // #ifndef _CPDBASE_INCLUDED
