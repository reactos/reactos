/*****************************************************************/ 
/**                  Microsoft Windows for Workgroups                **/
/**              Copyright (C) Microsoft Corp., 1991-1992            **/
/*****************************************************************/ 

/*
    npstring.h
    String classes: definition

    This file contains the basic string classes for the Thor UI.
    Its requirements are:

        - provide a modestly object-oriented interface to string
          manipulation, the better to work with the rest of our code;

        - encapsulate NLS and DBCS support as much as possible;

        - ensure that apps get correct form of library support,
          particularly with possible interference from intrinsics;

    The current solution consists of two classes: NLS_STR, and ISTR.

    class NLS_STR:      use wherever NLS/DBCS support is needed.
                      Most strings (in the UI, anyway) should be
                      of this class.

    class ISTR:      Indexes an NLS_STR in a DBCS safe manner.  All
                  positioning within an NLS_STR is done with ISTRs

    The class hierarchy looks like:

        BASE
            NLS_STR
                RESOURCE_STR
        ISTR

    This file also contains the STACK_NLS_STR macro and the
    strcpy( CHAR *, const NLS_STR& ) prototype.

    FILE HISTORY:
        beng        10/21/90        Created from email memo of last week
        johnl        11/13/90        Removed references to EB_STRING
        johnl        11/28/90        Release fully functional version
        johnl        12/07/90        Numerous revisions after code review
                                    (Removed SZ_STR, ISTR must associate
                                    w/a string upon decl etc.)
        beng        02/05/91        Replaced PCH with CHAR * for
                                    const-placement
        beng        04/26/91        Expunged of CB, IB types; relocated
                                    inline functions to string/strmisc.cxx
        beng        07/23/91        Added more *_STR types
        gregj        03/22/93        Ported to Chicago environment.
        gregj        03/25/93        Added Party(), DonePartying()
        gregj        03/30/93        Allow assigning NLS_STR to ISTR
        gregj        04/02/93        Added NLS_STR::IsDBCSLeadByte()
        gregj        04/02/93        Added ISTR::operator int
        gregj        04/08/93        Added NLS_STR::strncpy()
        gregj        04/08/93        Added NLS_STR::GetPrivateProfileString()
*/

#define WIN31    /* for certain string and NETLIB stuff */

#ifndef _BASE_HXX_
#include "base.h"
#endif

#ifndef _STRING_HXX_
#define _STRING_HXX_

extern HINSTANCE hInstance;        // for NLS_STR::LoadString

// String class doesn't allocate or deallocate memory
// for STR_OWNERALLOC strings
//
#define STR_OWNERALLOC         0x8000

// Same as owner alloc only the string is initialized with the null string.
//
#define STR_OWNERALLOC_CLEAR 0x8001

// Maximum resource string size, owner alloced strings must be at least
// MAX_RES_STR_LEN, otherwise an error will occur.
//
#define MAX_RES_STR_LEN    255


// The maximum number of insert parameters the InsertParams method can
// handle
//
#define MAX_INSERT_PARAMS    9


/*************************************************************************

    NAME:    ISTR

    SYNOPSIS:    String index object, used in conjunction with NLS_STR

    INTERFACE:
        ISTR()    - this ISTR gets associated with
                  the passed string and can only be used
                  on this string (NOTE:  on non-debug
                  versions this turns into a nop, can still
                  be useful for decl. clarity however).

        ISTR()    - Initialize to passed ISTR;
                  this ISTR will be associated with
                  the same string that the passed ISTR
                  is associated with.

        operator=()   - Copy passed ISTR (see prev.)

        operator=()   - Associate ISTR with a new NLS_STR.

        operator-()   - Returns CB diff. between *this & Param.
                        (must both belong to the same string)

        operator++()  - Advance the ISTR to the next logical
                        character (use only where absolutely
                        necessary).  Stops at end of string

        operator+=()  - Advance the ISTR to the ith logical
                        character (call operator++ i times)
                        Stops at end of string.

        operator==()  - Returns TRUE if the two ISTRs point to
                        the same position in the string (causes
                        an assertion failure if the two ISTRs
                        don't point to the same string).

        operator>()   - Returns true of *this is greater then
                        the passed ISTR (i.e., further along
                        in the string).

        operator<()   - Same as operator>, only less then.

        Reset()       - Resets ISTR to beginning of string and
                        updates the ISTR version number with the
                        string's current version number

    private:
        QueryIB()     - Returns the index in bytes
        QueryPNLS()   - Returns the pointer to the NLS_STR this ISTR
                        references
        SetPNLS()     - Sets the pointer to point to the NLS_STR
                        this ISTR references

    DEBUG ONLY:
        QueryVersion()     - Gets the version number of
                           the string this ISTR is associated with
        SetVersion()     - Sets the version number of this ISTR.

    USES:

    CAVEATS:    Each NLS_STR has a version number associated with it.  When
                an operation is performed that modifies the string, the
                version number is updated.  It is invalid to use an ISTR
                after its associated NLS_STR has been modified (can use
                Reset to resync it with the NLS_STR, the index gets reset
                to zero).

                You must associate an NLS_STR with an ISTR at the
                declaration of the ISTR.

    NOTES:        The version checking and string association checking goes
                away in the non-debug version.

    HISTORY:
        johnl        11/16/90        Created
        johnl        12/07/90        Modified after code review
        gregj        03/30/93        Allow assigning NLS_STR to ISTR

**************************************************************************/

class ISTR
{
friend class NLS_STR;

public:
    ISTR( const ISTR& istr );
    ISTR( const NLS_STR& nls );
    ISTR& operator=( const ISTR& istr );
    ISTR& operator=( const NLS_STR& nls );

    INT operator-( const ISTR& istr ) const;

    ISTR& operator++();
    VOID operator+=( INT iChars );

    BOOL operator==( const ISTR& istr ) const;
    BOOL operator>( const ISTR& istr )  const;
    BOOL operator<( const ISTR& istr )  const;

    operator INT() const { return QueryIB(); }

    VOID Reset();

private:
    INT _ibString;        // Index (in bytes) into an NLS_STR
    NLS_STR *_pnls;        // Pointer to "owner" NLS

    INT QueryIB() const
        { return _ibString; }
    VOID SetIB( INT ib )
        { _ibString = ib; }

    const NLS_STR* QueryPNLS() const
        { return _pnls; }
    VOID SetPNLS( const NLS_STR * pnls )
        { _pnls = (NLS_STR*)pnls; }

#ifdef DEBUG
    // Version number of NLS_STR this ISTR is associated with
    //
    USHORT _usVersion;

    USHORT QueryVersion() const { return _usVersion; }
    VOID SetVersion( USHORT usVers ) { _usVersion = usVers; }
#endif
};


/*************************************************************************

    NAME:        NLS_STR (nls)

    SYNOPSIS:    Provide a better string abstraction than the standard ASCIIZ
                representation offered by C (and C++).  The abstraction is
                better mainly because it handles double-byte characters
                (DBCS) in the string and makes intelligent use of operator
                overloading.

    INTERFACE:    NLS_STR()        Construct a NLS_STR (initialized to a CHAR *,
                                NLS_STR or NULL).  Reports errors via BASE.

                ~NLS_STR()        Destructor

                operator=()        Assign one NLS_STR (or CHAR *) value
                                to another (old string is deleted, new
                                string is allocated and copies source)

                operator+=()    Concatenate with assignment (equivalent to
                                strcat - see strcat).

                operator==()    Compare two NLS_STRs for equality

                operator!=()    Compare two NLS_STRs for inequality

                QueryPch()        Access operator, returning a "char *"
                                aliased to the string.  DO NOT MODIFY
                                THE STRING USING THIS METHOD (or pass
                                it to procedures that might modify it).
                                Synonym: operator const CHAR *().

                operator[]()    Same as QueryPch, except the string
                                is offset by ISTR characters

                IsDBCSLeadByte()    Returns whether a byte is a lead byte,
                                according to the ANSI- or OEM-ness of the
                                string.

        C-runtime-style methods.

                strlen()        Return the length of the string in bytes,
                                less terminator.  Provided only for crt
                                compatibility; please use a Query method
                                if possible.

                strcat()        Append an NLS_STR.  Will cause *this to be
                                reallocated if the appended string is larger
                                then this->QueryCb() and this is not an
                                STR_OWNERALLOC string

                strncpy()        Copy a non-null-terminated string into an
                                NLS_STR.  DBCS-safe.  For similar functionality
                                with an NLS_STR as the source, use the sub-
                                string members.

                strcmp()        Compare two NLS_STRs
                stricmp()        "
                strncmp()        Compare a portion of two NLS_STRs
                strnicmp()        "

                strcspn()        Find first char in *this that is
                                a char in arg
                strspn()        Find first char in *this that is
                                not a char in arg

                strtok()        Returns a token from the string

                strstr()        Search for a NLS_STR.

                strchr()        Search for a CHAR from beginning.
                                Returns offset.
                strrchr()        Search for a CHAR from end.
                strupr()        Convert NLS_STR to upper case.
                atoi()            Returns integer numeric value
                atol()            Returns long value

                realloc()        Resize string preserving its contents

        Other methods.

                QueryAllocSize()    Returns total # of bytes allocated (i.e.,
                                    number new was called with, or size of
                                    memory block if STR_OWNERALLOC
                IsOwnerAlloc()        Returns TRUE if this string is an owner
                                    allocated string
                QuerySubStr()        Return a substring
                InsertStr()            Insert a NLS_STR at given index.
                DelSubStr()            Delete a substring
                ReplSubStr()        Replace a substring (given start and
                                    NLS_STR)

                InsertParams()        Replace %1-%9 params in *this with the
                                    corresponding NLS_STRs contained in the
                                    array of pointers

                LoadString()        Load the string associated with the passed
                                    resource into *this (OWNER_ALLOC strings must
                                    be at least MAX_RES_STR_LEN).  Optionally
                                    calls InsertParams with a passed array of
                                    nls pointers.

                GetPrivateProfileString()    Loads a string from an INI file.

                Reset()                After an operation fails (due to memory
                                    failure), it is invalid to use the string
                                    until Reset has been called.  If the object
                                    wasn't successfully constructed, Reset will
                                    fail.

                QueryTextLength()    Returns the number of CHARS, less
                                    terminator.

                QueryTextSize()        Returns the number of bytes, including
                                    terminator.  Denotes amount of storage
                                    needed to dup string into a bytevector.

                QueryNumChar()        Returns total number of logical characters
                                    within the string.  Rarely needed.

                Append()            Appends a string to the current string,
                                    like strcat.
                AppendChar()        Appends a single character.

                Compare()            As strcmp().

                CopyFrom()            As operator=(), but returns an APIERR.

                ToOEM()                Convert string to OEM character set.

                ToAnsi()            Convert string to ANSI character set.

                Party()                Obtain read-write access to the buffer.
                DonePartying()        Release read-write access.

    PARENT:    BASE

    USES:    ISTR

    CAVEATS:    A NLS_STR object can enter an error state for various
                reasons - typically a memory allocation failure.  Using
                an object in such a state is theoretically an error.
                Since string operations frequently occur in groups,
                we define operations on an erroneous string as no-op,
                so that the check for an error may be postponed until
                the end of the complex operation.  Most member functions
                which calculate a value will treat an erroneous string
                as having zero length; however, clients should not depend
                on this.

                Attempting to use an ISTR that is registered with another
                string will cause an assertion failure.

                Each NLS_STR has a version/modification flag that is also
                stored in the the ISTR.  An attempt to use an ISTR on an
                NLS_STR that has been modified (by calling one of the methods
                listed below) will cause an assertion failure.    To use the
                ISTR after a modifying method, you must first call ISTR::Reset
                which will update the version in the ISTR.  See the
                method definition for more detail.

                List of modifying methods:
                    All NLS::operator= methods
                    NLS::DelSubStr
                    NLS::ReplSubStr
                    NLS::InsertSubStr

                NOTE: The ISTR used as a starting index on the
                Substring methods remains valid after the call.

                Party() and DonePartying() can be used when you need to
                do something that the standard NLS_STR methods don't
                cover.  For example, you might want to tweak the first
                couple of characters in a pathname;  if you know they're
                definitely not double-byte characters, this is safe to
                do with ordinary character assignments.
                
                Calling Party() returns a pointer to the string buffer,
                and places the NLS_STR in an error state to prevent the
                standard methods from operating on it (and thereby getting
                confused by the possibly incorrect cached length).  You
                can still call QueryAllocSize() to find out the maximum
                size of the buffer.  When you've finished, call DonePartying()
                to switch the NLS_STR back to "normal" mode.  There are two
                overloaded forms of DonePartying().  If you know what the
                length of the string is, you can pass it in, and NLS_STR
                will just use that.  Otherwise, it will use strlenf() to
                find out what the new length is.  If you don't plan to
                change the length, call strlen() on the string before you
                Party(), save that length, and pass it to DonePartying().
                The initial strlen() is fast because it's cached.

                If you find yourself constantly Party()ing in order to
                accomplish a particular function, that function should
                be formally added to the NLS_STR definition.

    NOTES:        The lack of a strlwr() method comes from a shortcoming
                in the casemap tables.    Sorry.

                STR_OWNERALLOC strings are a special type of NLS_STR
                You mark a string as STR_OWNERALLOC on
                construction by passing the flag STR_OWNERALLOC and a pointer
                to your memory space where you want the string to reside
                plus the size of the memory block.
                THE POINTER MUST POINT TO A VALID NULL TERMINATED STRING.
                You are guaranteed this pointer will never be
                resized or deleted.  Note that no checking is performed for
                writing beyond the end of the string.  Valid uses include
                static strings, severe optimization, owner controlled
                memory allocation or stack controlled memory allocation.

                BUGBUG: should make IsOwnerAlloc and QueryAllocSize
                protected.  (beng 22-Jul-1991)

                CODEWORK: Owner-alloc strings should be a distinct
                class from these normal strings.

                CODEWORK: Should add a fReadOnly flag.

                CODEWORK: Should clean up this mess, and make the
                owner-alloc constructor protected.

                I wish I could clean up this mess...

    HISTORY:
        johnl        11/28/90        First fully functioning version
        johnl        12/07/90        Incorporated code review changes
        terryk        04/05/91        add QueryNumChar method
        beng        07/22/91        Added more methods; separated fOwnerAlloc
                                    from cbData
        gregj        05/22/92        Added ToOEM, ToAnsi methods
        gregj        03/22/93        Ported to Chicago environment
        gregj        04/02/93        Added IsDBCSLeadByte()
        gregj        04/08/93        Added strncpy()

**************************************************************************/

class NLS_STR : public BASE
{
friend class ISTR; // Allow access to CheckIstr

public:
    // Default constructor, creating an empty string.
    //
    NLS_STR();

    // Initialize to "cchInitLen" characters, each "chInit",
    // plus trailing NUL.
    //
    NLS_STR( INT cchInitLen );

    // Initialize from a NUL-terminated character vector.
    //
    NLS_STR( const CHAR *pchInit );

    // Initialize an NLS_STR to memory position passed in achInit
    // No memory allocation of any type will be performed on this string
    // cbSize should be the total memory size of the buffer, if cbSize == -1
    // then the size of the buffer will assumed to be strlen(achInit)+1
    //
    NLS_STR( unsigned stralloc, CHAR *pchInit, INT cbSize = -1 );

    // Initialize from an existing x_STRING.
    //
    NLS_STR( const NLS_STR& nlsInit );

    ~NLS_STR();

    // Number of bytes the string uses (not including terminator)
    // Cf. QueryTextLength and QueryTextSize.
    //
    inline INT strlen() const;

    // Return a read-only CHAR vector, for the APIs.
    //
    const CHAR *QueryPch() const
#ifdef DEBUG
        ;
#else
        { return _pchData; }
#endif

    const CHAR *QueryPch( const ISTR& istr ) const
#ifdef DEBUG
        ;
#else
        { return _pchData + istr.QueryIB(); }
#endif

    WCHAR QueryChar( const ISTR& istr ) const
#ifdef DEBUG
        ;
#else
        { return *(_pchData+istr.QueryIB()); }
#endif

    operator const CHAR *() const
        { return QueryPch(); }

    const CHAR *operator[]( const ISTR& istr ) const
        { return QueryPch(istr); }

    BOOL IsDBCSLeadByte( CHAR ch ) const;

    // Total allocated storage
    //
    inline INT QueryAllocSize() const;

    inline BOOL IsOwnerAlloc() const;

    // Increase the size of a string preserving its contents.
    // Returns TRUE if successful, false otherwise (illegal to
    // call for an owner alloced string).  If you ask for a string smaller
    // then the currently allocated one, the request will be ignored and TRUE
    // will be returned.
    //
    BOOL realloc( INT cbNew );

    // Returns TRUE if error was successfully cleared (string is now in valid
    // state), FALSE otherwise.
    //
    BOOL Reset();

    NLS_STR& operator=( const NLS_STR& nlsSource );
    NLS_STR& operator=( const CHAR *achSource );

    NLS_STR& operator+=( WCHAR wch );        // NEW, replaces AppendChar
    NLS_STR& operator+=( const NLS_STR& nls ) { return strcat(nls); }
    NLS_STR& operator+=( LPCSTR psz ) { return strcat(psz); }

    NLS_STR& strncpy( const CHAR *pchSource, UINT cbSource );

    NLS_STR& strcat( const NLS_STR& nls );
    NLS_STR& strcat( LPCSTR psz );

    BOOL operator== ( const NLS_STR& nls ) const;
    BOOL operator!= ( const NLS_STR& nls ) const;

    INT strcmp( const NLS_STR& nls ) const;
    INT strcmp( const NLS_STR& nls, const ISTR& istrThis ) const;
    INT strcmp( const NLS_STR& nls, const ISTR& istrThis,
                const ISTR& istrStart2 ) const;

    INT stricmp( const NLS_STR& nls ) const;
    INT stricmp( const NLS_STR& nls, const ISTR& istrThis ) const;
    INT stricmp( const NLS_STR& nls, const ISTR& istrThis,
                 const ISTR& istrStart2 ) const;

    INT strncmp( const NLS_STR& nls, const ISTR& istrLen ) const;
    INT strncmp( const NLS_STR& nls, const ISTR& istrLen,
                 const ISTR& istrThis ) const;
    INT strncmp( const NLS_STR& nls, const ISTR& istrLen,
                 const ISTR& istrThis, const ISTR& istrStart2 ) const;

    INT strnicmp( const NLS_STR& nls, const ISTR& istrLen ) const;
    INT strnicmp( const NLS_STR& nls, const ISTR& istrLen,
                  const ISTR& istrThis ) const;
    INT strnicmp( const NLS_STR& nls, const ISTR& istrLen,
                  const ISTR& istrThis, const ISTR& istrStart2 ) const;

    // The following str* functions return TRUE if successful (istrPos has
    // meaningful data), false otherwise.
    //
    BOOL strcspn( ISTR *istrPos, const NLS_STR& nls ) const;
    BOOL strcspn( ISTR *istrPos, const NLS_STR& nls, const ISTR& istrStart ) const;
    BOOL strspn( ISTR *istrPos, const NLS_STR& nls ) const;
    BOOL strspn( ISTR *istrPos, const NLS_STR& nls, const ISTR& istrStart ) const;

    BOOL strstr( ISTR *istrPos, const NLS_STR& nls ) const;
    BOOL strstr( ISTR *istrPos, const NLS_STR& nls, const ISTR& istrStart ) const;

    BOOL stristr( ISTR *istrPos, const NLS_STR& nls ) const;
    BOOL stristr( ISTR *istrPos, const NLS_STR& nls, const ISTR& istrStart ) const;

    BOOL strchr( ISTR *istrPos, const CHAR ch ) const;
    BOOL strchr( ISTR *istrPos, const CHAR ch, const ISTR& istrStart ) const;

    BOOL strrchr( ISTR *istrPos, const CHAR ch ) const;
    BOOL strrchr( ISTR *istrPos, const CHAR ch, const ISTR& istrStart ) const;

    BOOL strtok( ISTR *istrPos, const NLS_STR& nlsBreak, BOOL fFirst = FALSE );

    LONG atol() const;
    LONG atol( const ISTR& istrStart ) const;

    INT atoi() const;
    INT atoi( const ISTR& istrStart ) const;

    NLS_STR& strupr();

    // Return a pointer to a new NLS_STR that contains the contents
    // of *this from istrStart to:
    //        End of string if no istrEnd is passed
    //        istrStart + istrEnd
    //
    NLS_STR *QuerySubStr( const ISTR& istrStart ) const;
    NLS_STR *QuerySubStr( const ISTR& istrStart, const ISTR& istrEnd ) const;

    // Collapse the string by removing the characters from istrStart to:
    //        End of string
    //        istrStart + istrEnd
    // The string is not reallocated
    //
    VOID DelSubStr( ISTR& istrStart );
    VOID DelSubStr( ISTR& istrStart, const ISTR& istrEnd );

    BOOL InsertStr( const NLS_STR& nlsIns, ISTR& istrStart );

    // Replace till End of string of either *this or replacement string
    // (or istrEnd in the 2nd form) starting at istrStart
    //
    VOID ReplSubStr( const NLS_STR& nlsRepl, ISTR& istrStart );
    VOID ReplSubStr( const NLS_STR& nlsRepl, ISTR& istrStart,
                     const ISTR& istrEnd );

    // Replace %1-%9 in *this with corresponding index from apnlsParamStrings
    // Ex. if *this="Error %1" and apnlsParamStrings[0]="Foo" the resultant
    //       string would be "Error Foo"
    //
    USHORT InsertParams( const NLS_STR *apnlsParamStrings[] );

    // Load a message from a resource file into *this (if string is an
    // OWNER_ALLOC string, then must be at least MAX_RES_STR_LEN.  Heap
    // NLS_STRs will be reallocated if necessary
    //
    USHORT LoadString( USHORT usMsgID );

    // Combines functionality of InsertParams & LoadString.  *this gets loaded
    // with the string from the resource file corresponding to usMsgID.
    //
    USHORT LoadString( USHORT usMsgID, const NLS_STR *apnlsParamStrings[] );

    VOID GetPrivateProfileString( const CHAR *pszFile, const CHAR *pszSection,
                                  const CHAR *pszKey, const CHAR *pszDefault = NULL );

    VOID ToOEM();            // convert ANSI to OEM

    VOID ToAnsi();            // convert OEM to ANSI

    VOID SetOEM();            // declare that string was constructed as OEM
    VOID SetAnsi();            // declare that string was constructed as ANSI

    inline BOOL IsOEM() const;

    CHAR *Party();            // get read-write access
    VOID DonePartying( VOID );            // if you don't have the length handy
    VOID DonePartying( INT cchNew );    // if you do

#ifdef EXTENDED_STRINGS
    // Initialize from a NUL-terminated character vector
    // and allocate a minimum of: cbTotalLen+1 bytes or strlen(achInit)+1
    //
    NLS_STR( const CHAR *pchInit, INT iTotalLen );

    // Similar to prev. except the string pointed at by pchInit is copied
    // to pchBuff.  The address of pchBuff is used as the string storage.
    // cbSize is required.  stralloc can only be STR_OWNERALLOC (it makes
    // no sense to use STR_OWNERALLOC_CLEAR).
    //
    NLS_STR( unsigned stralloc, CHAR *pchBuff, INT cbSize,
             const CHAR *pchInit );

    // return the number of logical characters within the string
    //
    INT QueryNumChar() const;

    // Return the number of printing CHARs in the string.
    // This number does not include the termination character.
    //
    // Cf. QueryNumChar, which returns a count of glyphs.
    //
    INT QueryTextLength() const;

    // Return the number of BYTES occupied by the string's representation.
    // Cf. QueryAllocSize, which returns the total amount alloc'd.
    //
    INT QueryTextSize() const;

    APIERR Append( const NLS_STR& nls );

    APIERR AppendChar( WCHAR wch );

    APIERR CopyFrom( const NLS_STR& nlsSource );
    APIERR CopyFrom( const CHAR *achSource );

    INT Compare( const NLS_STR *nls ) const { return strcmp(*nls); }

#endif

private:
    UINT _fsFlags;        // owner-alloc, character set flags
#define SF_OWNERALLOC    0x1
#define SF_OEM            0x2

    INT _cchLen;        // Number of bytes string uses (strlen)
    INT _cbData;        // Total storage allocated
    CHAR *_pchData;        // Pointer to Storage

#ifdef DEBUG
    USHORT _usVersion;    // Version count (inc. after each change)
#endif

    // The following substring functions are used internally (can't be
    // exposed since they take an INT cbLen parameter for an index).
    //
    VOID DelSubStr( ISTR&istrStart, INT cbLen );

    NLS_STR *QuerySubStr( const ISTR& istrStart, INT cbLen ) const;

    VOID ReplSubStr( const NLS_STR& nlsRepl, ISTR& istrStart, INT cbLen );

    BOOL Alloc( INT cchLen );      // Allocate memory for a string

#ifdef DEBUG        // DEBUG is new for these
    // CheckIstr checks whether istr is associated with this, asserts out
    // if it is not.  Also checks version numbers in debug version.
    //
    VOID CheckIstr( const ISTR& istr ) const;

    // UpdateIstr syncs the version number between *this and the passed
    // ISTR.  This is for operations that cause an update to the string
    // but the ISTR that was passed in is still valid (see InsertSubSt).
    //
    VOID UpdateIstr( ISTR *pistr ) const;

    // IncVers adds one to this strings version number because the previous
    // operation caused the contents to change thus possibly rendering
    // ISTRs on this string as invalid.
    //
    VOID IncVers();

    // InitializeVers sets the version number to 0
    //
    VOID InitializeVers();

    // QueryVersion returns the current version number of this string
    //
    USHORT QueryVersion() const;
#else    // DEBUG
    VOID CheckIstr( const ISTR& istr ) const { }
    VOID UpdateIstr( ISTR *pistr ) const { }
    VOID IncVers() { }
    VOID InitializeVers() { }
    USHORT QueryVersion() const { return 0; }
#endif
};


/***********************************************************************/

/***********************************************************************
 *
 *  Macro STACK_NLS_STR(name, len )
 *
 *    Define an NLS string on the stack with the name of "name" and the
 *    length of "len".    The strlen will be 0 and the first character will
 *    be '\0'.    One byte is added for the NULL terminator.  Usage:
 *        STACK_NLS_STR( UncPath, UNCLEN );
 *
 *  Macro ISTACK_NLS_STR(name, len, pchInitString )
 *
 *    Same as STACK_NLS_STR except ISTACK_NLS_STR takes an initializer.
 **********************************************************************/

#define STACK_NLS_STR( name, len )                \
    CHAR _tmp##name[ len+1 ] ;                    \
    *_tmp##name = '\0' ;                    \
    NLS_STR name( STR_OWNERALLOC, _tmp##name, len+1 );

#define ISTACK_NLS_STR( name, len, pchInitString )        \
    STACK_NLS_STR( name, len ) ;                \
    name = pchInitString;

/***********************************************************************/

BOOL NLS_STR::IsOwnerAlloc() const
{
    return _fsFlags & SF_OWNERALLOC;
}

BOOL NLS_STR::IsOEM() const
{
    return _fsFlags & SF_OEM;
}

INT NLS_STR::strlen() const
{
    return _cchLen;
}

INT NLS_STR::QueryAllocSize()  const
{
    return _cbData;
}

#endif // _STRING_HXX_
