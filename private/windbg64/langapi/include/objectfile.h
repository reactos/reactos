//-----------------------------------------------------------------------------
//	ObjectFile.h
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  Purpose:
//		Define the classes for reading and groking object files
//
//  Revision History:
//
//	[]		10-Feb-1997 Dans	Created
//
//-----------------------------------------------------------------------------
#pragma once

#if !defined(_objectfile_h)
#define _objectfile_h 1

#include "crefobj.h"
#include "ref.h"
#include "buffer.h"
#include "simparray.h"

#if !defined(OBJECTFILE_IMPL)
#define IMPORT_EXPORT   __declspec(dllimport)
#else
#define IMPORT_EXPORT   __declspec(dllexport)
#endif

typedef IMAGE_FILE_HEADER       ImgFileHdr;
typedef ImgFileHdr *            PImgFileHdr;
typedef const PImgFileHdr       PCImgFileHdr;

typedef IMAGE_SECTION_HEADER    ImgSectHdr;
typedef ImgSectHdr *            PImgSectHdr;
typedef const ImgSectHdr *      PCImgSectHdr;

typedef IMAGE_RELOCATION        ImgReloc;
typedef ImgReloc *              PImgReloc;
typedef const ImgReloc *        PCImgReloc;

typedef IMAGE_LINENUMBER        ImgLineNo;
typedef PIMAGE_LINENUMBER       PImgLineNo;

typedef IMAGE_SYMBOL            ImgSym;
typedef ImgSym *                PImgSym;
typedef const ImgSym *          PCImgSym;

typedef IMAGE_AUX_SYMBOL        ImgAuxSym;
typedef ImgAuxSym *             PImgAuxSym;
typedef const ImgAuxSym *       PCImgAuxSym;

typedef BYTE *                  PB;
typedef const PB                PCB;

// symbol index (not COFF symbol index)
typedef DWORD                   SYMI;
const SYMI                      symiNil = 0;

// coff symbol table index
typedef DWORD                   COFFSYMI;
const COFFSYMI                  coffsymiNil = 0;


// munged info for fixups.  
struct FixupMap {
    DWORD   off;    // offset of fixup (in the section)
    DWORD   cb;     // how many bytes the fixup consumes in the data
    };

typedef SimpleArray<BYTE>       RGBYTE;
typedef SimpleArray<FixupMap>   RGFIXUPMAP;
typedef SimpleString            RGCH;

#if !defined(pure)
#define pure = 0
#endif

interface EnumSection;
interface EnumRelocation;
interface EnumImgSymbol;
typedef int             ISection;
const ISection          iSectionNil = 0;

// Dump flags
enum ODF {          // Object Dump Flags
    odfHdrs     = 0x01,
    odfSects    = 0x02,
    odfFixups   = 0x04,
    odfSyms     = 0x08,
    odfLineNum  = 0x10,
    odfAll      = (odfHdrs | odfSects | odfFixups | odfSyms | odfLineNum),
    odfFmtLong  = 0x20,
    odfFmtShort = 0x40,
    odfFmtByte  = 0x80
    };


// debugger callback to provide address for a symbol name.
// will also be responsible for indirecting the function pointers
// through the ilink thunks
typedef DWORD (__stdcall * PfnAddressFromName)(LPCTSTR);
typedef bool (__stdcall * PfnFSecOffFromName)(LPCTSTR, SHORT *, DWORD *);

// define the interface class to the rest of the world.
class ObjectCode : public CRefCountedObj {
    
public:
    // provide static creation methods so we don't have to expose operator
    // new/delete semantics
    IMPORT_EXPORT static bool
    FCreate ( RefPtr<ObjectCode> &, DWORD dwMachineTypeExpected, LPCTSTR szFileName );

    IMPORT_EXPORT static bool
    FCreate ( RefPtr<ObjectCode> &, DWORD dwMachineTypeExpected, HFILE hFile, DWORD offset =0 );

    // get the symbol index for a particular name
    virtual SYMI
    SymiFromSz ( LPCTSTR szName ) const pure;

    // get the name from a particular symbol index
    virtual LPCTSTR
    SzFromSymi ( SYMI symi ) const pure;

    virtual SYMI
    SymiFromCoffsymi ( COFFSYMI coffsymi ) const pure;

    // get a PImgSym from a coff symbol index
    virtual PCImgSym
    PCImgSymFromCoffsymi ( COFFSYMI coffsymi ) const pure;

    virtual bool
    FSectionFromSzSymbol (
        LPCTSTR             szSymbolName,
        PCImgSectHdr &      pcImgSectHdr,
        RefPtr<RGBYTE> &    rgbSectdata,
        RefPtr<RGFIXUPMAP> &rgfixup
        ) const pure;

    // get the count of sections
    virtual DWORD
    CSections ( ) const pure;

    // get the count of image symbols
    virtual DWORD
    CImgSym ( ) const pure;

    // get a section enumerator
    virtual bool
    FGetEnumSection ( EnumSection ** ) const pure;

    // get a symbol enumerator
    virtual bool
    FGetEnumImgSymbol ( EnumImgSymbol ** ) const pure;

    // apply fixups to some data
    virtual bool
    FApplyFixups ( PB, CB, DWORD, DWORD, PImgReloc, DWORD, PfnAddressFromName, PfnFSecOffFromName, SHORT, bool) const pure;

    // get the name of the object
    virtual LPCTSTR
    SzName ( ) const pure;

    // get the name of a symbol
    virtual LPCTSTR
    SzSymbol ( PCImgSym ) const pure;

    // get comdat symbol name defined by isection
    virtual void
    SzSymForIsec ( RGCH&, ISection ) const pure;

    virtual const char *
    SzRelocationType ( WORD, WORD *, bool * ) const pure;

    // dump an object
    virtual void
    Dump ( ODF ) const pure;

   };


// define the Section interface class to the rest of the world.
class Section : public CRefCountedObj {

public:
    virtual bool
    FInit ( ObjectCode *, PImgSectHdr, SYMI, PB, DWORD, ISection ) pure;

    virtual DWORD
    CRelocation ( ) const pure;

    // get a relocation enumerator
    virtual bool
    FGetEnumRelocation ( EnumRelocation ** ) const pure;

    // get the name of the section
    virtual char *
    SzName ( RefPtr<RGCH>& szName ) const pure;

    virtual DWORD
    CLineNum ( ) const pure;

    virtual DWORD
    CbRawData ( ) const pure;

	virtual bool
	FRawData( RefPtr<RGBYTE>& rgbSectdata ) const pure;

    virtual void
    LoadData ( void * ) const pure;

    virtual bool
    FApplyFixups ( DWORD, DWORD, PfnAddressFromName, PfnFSecOffFromName, SHORT ) pure;

    virtual void
    LoadFixupMap ( void * ) const pure;

    virtual void
    LoadLineNumbers ( void * ) const pure;

    virtual void
    Dump ( ODF ) const pure;

    };
    
typedef Section *       PSection;

// section enumerator
interface EnumSection : public Enum {
    virtual void get( Section ** const ) pure;
};

// relocation/fixup enumerator
interface EnumRelocation : public Enum {
    virtual void get( PImgReloc * const ) pure;
};

// symbol enumerator
interface EnumImgSymbol : public Enum {
    virtual void get ( PImgSym * const ) pure;
};


#endif
