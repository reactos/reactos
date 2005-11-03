/*
 * MS debug information definitions.
 *
 * Copyright (C) 1996 Eric Youngdale
 * Copyright (C) 1999-2000 Ulrich Weigand
 * Copyright (C) 2004 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* MS has stored all its debug information in a set of structures
 * which has been rather consistent across the years (ie you can grasp
 * some continuity, and not so many drastic changes).
 *
 * A bit of history on the various formats
 *      MSVC 1.0        PDB v1 (new format for debug info)
 *      MSVC 2.0        Inclusion in link of debug info (PDB v2)
 *      MSVC 5.0        Types are 24 bits (instead of 16 for <= 4.x)
 *      MSVC x.0        PDB (change in internal streams layout)
 *        
 *      .DBG            Contains COFF, FPO and Codeview info
 *      .PDB            New format for debug info (information is
 *                      derived from Codeview information)
 *      VCx0.PDB        x major MSVC number, stores types, while
 *                      <project>.PDB stores symbols.
 *
 * Debug information can either be found in the debug section of a PE
 * module (in something close to a .DBG file), or the debug section
 * can actually refer to an external file, which can be in turn,
 * either a .DBG or .PDB file. 
 *
 * Regarding PDB files:
 * -------------------
 * They are implemented as a set of internal files (as a small file
 * system). The file is split into blocks, an internal file is made
 * of a set of blocks. Internal files are accessed through
 * numbers. For example, 
 * 1/ is the ROOT (basic information on the file)
 * 2/ is the Symbol information (global symbols, local variables...)
 * 3/ is the Type internal file (each the symbols can have type
 * information associated with it).
 *
 * Over the years, three formats existed for the PDB:
 * - ?? was rather linked to 16 bit code (our support shall be rather
 *   bad)
 * - JG: it's the signature embedded in the file header. This format
 *   has been used in MSVC 2.0 => 5.0.
 * - DS: it's the signature embedded in the file header. It's the
 *   current format supported my MS.
 *
 * Types internal stream
 * ---------------------
 * Types (from the Type internal file) have existed in three flavors
 * (note that those flavors came as historical evolution, but there
 * isn't a one to one link between types evolution and PDB formats'
 * evolutions:
 * - the first flavor (suffixed by V1 in this file), where the types
 *   and subtypes are 16 bit entities; and where strings are in Pascal
 *   format (first char is their length and are not 0 terminated) 
 * - the second flavor (suffixed by V2) differs from first flavor with
 *   types and subtypes as 32 bit entities. This forced some
 *   reordering of fields in some types
 * - the third flavor (suffixed by V3) differs from second flavor with
 *   strings stored as C strings (ie are 0 terminated, instead of
 *   length prefixed)
 * The different flavors can coexist in the same file (is this really
 * true ??)
 * 
 * For the evolution of types, the need of the second flavor was the
 * number of types to be defined (limited to 0xFFFF, including the C
 * basic types); the need of the third flavor is the increase of
 * symbol size (to be greated than 256), which was likely needed for
 * complex C++ types (nested + templates).
 *
 * It's somehow difficult to represent the layout of those types on
 * disk because:
 * - some integral values are stored as numeric leaf, which size is
 *   variable depending on its value
 * 
 * Symbols internal stream
 * -----------------------
 * Here also we find three flavors (that we've suffixed with _V1, _V2
 * and _V3) even if their evolution is closer to the evolution of
 * types, there are not completly linked together.
 */

#include "pshpack1.h"

/* ======================================== *
 *             Type information
 * ======================================== */

struct p_string
{
    unsigned char               namelen;
    char                        name[1];
};

union codeview_type
{
    struct
    {
        unsigned short int      len;
        short int               id;
    } generic;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               attribute;
        short int               type;
    } modifier_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        int                     type;
        short int               attribute;
    } modifier_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               attribute;
        short int               datatype;
        struct p_string         p_name;
    } pointer_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            datatype;
        unsigned int            attribute;
        struct p_string         p_name;
    } pointer_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned char           nbits;
        unsigned char           bitoff;
        unsigned short          type;
    } bitfield_v1;
    
    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            type;
        unsigned char           nbits;
        unsigned char           bitoff;
    } bitfield_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               elemtype;
        short int               idxtype;
        unsigned short int      arrlen;     /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } array_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            elemtype;
        unsigned int            idxtype;
        unsigned short int      arrlen;    /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } array_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            elemtype;
        unsigned int            idxtype;
        unsigned short int      arrlen;    /* numeric leaf */
#if 0
        char                    name[1];
#endif
    } array_v3;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               n_element;
        short int               fieldlist;
        short int               property;
        short int               derived;
        short int               vshape;
        unsigned short int      structlen;  /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } struct_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               n_element;
        short int               property;
        unsigned int            fieldlist;
        unsigned int            derived;
        unsigned int            vshape;
        unsigned short int      structlen;  /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } struct_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               n_element;
        short int               property;
        unsigned int            fieldlist;
        unsigned int            derived;
        unsigned int            vshape;
        unsigned short int      structlen;  /* numeric leaf */
#if 0
        char                    name[1];
#endif
    } struct_v3;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               count;
        short int               fieldlist;
        short int               property;
        unsigned short int      un_len;     /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } union_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               count;
        short int               property;
        unsigned int            fieldlist;
        unsigned short int      un_len;     /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } union_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               count;
        short int               property;
        unsigned int            fieldlist;
        unsigned short int      un_len;     /* numeric leaf */
#if 0
        char                    name[1];
#endif
    } union_v3;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               count;
        short int               type;
        short int               field;
        short int               property;
        struct p_string         p_name;
    } enumeration_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               count;
        short int               property;
        unsigned int            type;
        unsigned int            field;
        struct p_string         p_name;
    } enumeration_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               count;
        short int               property;
        unsigned int            type;
        unsigned int            field;
        char                    name[1];
    } enumeration_v3;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned char           list[1];
    } fieldlist;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      rvtype;
        unsigned char           call;
        unsigned char           reserved;
        unsigned short int      params;
        unsigned short int      arglist;
    } procedure_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            rvtype;
        unsigned char           call;
        unsigned char           reserved;
        unsigned short int      params;
        unsigned int            arglist;
    } procedure_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      rvtype;
        unsigned short int      class_type;
        unsigned short int      this_type;
        unsigned char           call;
        unsigned char           reserved;
        unsigned short int      params;
        unsigned short int      arglist;
        unsigned int            this_adjust;
    } mfunction_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned                unknown1; /* could be this_type ??? */
        unsigned int            class_type;
        unsigned int            rvtype;
        unsigned char           call;
        unsigned char           reserved;
        unsigned short          params;
        unsigned int            arglist;
        unsigned int            this_adjust;
    } mfunction_v2;
};

union codeview_fieldtype
{
    struct
    {
        short int		id;
    } generic;

    struct
    {
        short int		id;
        short int		type;
        short int		attribute;
        unsigned short int	offset;     /* numeric leaf */
    } bclass_v1;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        unsigned short int	offset;     /* numeric leaf */
    } bclass_v2;

    struct
    {
        short int		id;
        short int		btype;
        short int		vbtype;
        short int		attribute;
        unsigned short int	vbpoff;     /* numeric leaf */
#if 0
        unsigned short int	vboff;      /* numeric leaf */
#endif
    } vbclass_v1;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        btype;
        unsigned int	        vbtype;
        unsigned short int	vbpoff;     /* numeric leaf */
#if 0
        unsigned short int	vboff;      /* numeric leaf */
#endif
    } vbclass_v2;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned short int	value;     /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } enumerate_v1;

   struct
    {
        short int               id;
        short int               attribute;
        unsigned short int      value;     /* numeric leaf */
#if 0
        char                    name[1];
#endif
    } enumerate_v3;

    struct
    {
        short int		id;
        short int		type;
        struct p_string         p_name;
    } friendfcn_v1;

    struct
    {
        short int		id;
        short int		_pad0;
        unsigned int	        type;
        struct p_string         p_name;
    } friendfcn_v2;

    struct
    {
        short int		id;
        short int		type;
        short int		attribute;
        unsigned short int	offset;    /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } member_v1;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        unsigned short int	offset;    /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } member_v2;

    struct
    {
        short int               id;
        short int               attribute;
        unsigned int            type;
        unsigned short int      offset; /* numeric leaf */
#if 0
        unsigned char           name[1];
#endif
    }
    member_v3;

    struct
    {
        short int		id;
        short int		type;
        short int		attribute;
        struct p_string         p_name;
    } stmember_v1;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        struct p_string         p_name;
    } stmember_v2;

    struct
    {
        short int		id;
        short int		count;
        short int		mlist;
        struct p_string         p_name;
    } method_v1;

    struct
    {
        short int		id;
        short int		count;
        unsigned int	        mlist;
        struct p_string         p_name;
    } method_v2;

    struct
    {
        short int		id;
        short int		index;
        struct p_string         p_name;
    } nesttype_v1;

    struct
    {
        short int		id;
        short int		_pad0;
        unsigned int	        index;
        struct p_string         p_name;
    } nesttype_v2;

    struct
    {
        short int		id;
        short int		type;
    } vfunctab_v1;

    struct
    {
        short int		id;
        short int		_pad0;
        unsigned int	        type;
    } vfunctab_v2;

    struct
    {
        short int		id;
        short int		type;
    } friendcls_v1;

    struct
    {
        short int		id;
        short int		_pad0;
        unsigned int	        type;
    } friendcls_v2;

    struct
    {
        short int		id;
        short int		attribute;
        short int		type;
        struct p_string         p_name;
    } onemethod_v1;

    struct
    {
        short int		id;
        short int		attribute;
        short int		type;
        unsigned int	        vtab_offset;
        struct p_string         p_name;
    } onemethod_virt_v1;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int 	        type;
        struct p_string         p_name;
    } onemethod_v2;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        unsigned int	        vtab_offset;
        struct p_string         p_name;
    } onemethod_virt_v2;

    struct
    {
        short int		id;
        short int		type;
        unsigned int	        offset;
    } vfuncoff_v1;

    struct
    {
        short int		id;
        short int		_pad0;
        unsigned int	        type;
        unsigned int	        offset;
    } vfuncoff_v2;

    struct
    {
        short int		id;
        short int		attribute;
        short int		index;
        struct p_string         p_name;
    } nesttypeex_v1;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        index;
        struct p_string         p_name;
    } nesttypeex_v2;

    struct
    {
        short int		id;
        short int		attribute;
        unsigned int	        type;
        struct p_string         p_name;
    } membermodify_v2;

};


/*
 * This covers the basic datatypes that VC++ seems to be using these days.
 * 32 bit mode only.  There are additional numbers for the pointers in 16
 * bit mode.  There are many other types listed in the documents, but these
 * are apparently not used by the compiler, or represent pointer types
 * that are not used.
 */
#define T_NOTYPE	        0x0000	/* Notype */
#define T_ABS		        0x0001	/* Abs */
#define T_VOID		        0x0003	/* Void */
#define T_CHAR		        0x0010	/* signed char */
#define T_SHORT		        0x0011	/* short */
#define T_LONG		        0x0012	/* long */
#define T_QUAD		        0x0013	/* long long */
#define T_UCHAR		        0x0020	/* unsigned  char */
#define T_USHORT	        0x0021	/* unsigned short */
#define T_ULONG		        0x0022	/* unsigned long */
#define T_UQUAD		        0x0023	/* unsigned long long */
#define T_REAL32	        0x0040	/* float */
#define T_REAL64	        0x0041	/* double */
#define T_RCHAR		        0x0070	/* real char */
#define T_WCHAR		        0x0071	/* wide char */
#define T_INT4		        0x0074	/* int */
#define T_UINT4		        0x0075	/* unsigned int */

#define T_32PVOID	        0x0403	/* 32 bit near pointer to void */
#define T_32PCHAR	        0x0410  /* 16:32 near pointer to signed char */
#define T_32PSHORT	        0x0411  /* 16:32 near pointer to short */
#define T_32PLONG	        0x0412  /* 16:32 near pointer to int */
#define T_32PQUAD	        0x0413  /* 16:32 near pointer to long long */
#define T_32PUCHAR	        0x0420  /* 16:32 near pointer to unsigned char */
#define T_32PUSHORT	        0x0421  /* 16:32 near pointer to unsigned short */
#define T_32PULONG	        0x0422	/* 16:32 near pointer to unsigned int */
#define T_32PUQUAD	        0x0423  /* 16:32 near pointer to long long */
#define T_32PREAL32	        0x0440	/* 16:32 near pointer to float */
#define T_32PREAL64	        0x0441	/* 16:32 near pointer to float */
#define T_32PRCHAR	        0x0470	/* 16:32 near pointer to real char */
#define T_32PWCHAR	        0x0471	/* 16:32 near pointer to real char */
#define T_32PINT4	        0x0474	/* 16:32 near pointer to int */
#define T_32PUINT4	        0x0475  /* 16:32 near pointer to unsigned int */


#define LF_MODIFIER_V1          0x0001
#define LF_POINTER_V1           0x0002
#define LF_ARRAY_V1             0x0003
#define LF_CLASS_V1             0x0004
#define LF_STRUCTURE_V1         0x0005
#define LF_UNION_V1             0x0006
#define LF_ENUM_V1              0x0007
#define LF_PROCEDURE_V1         0x0008
#define LF_MFUNCTION_V1         0x0009
#define LF_VTSHAPE_V1           0x000a
#define LF_COBOL0_V1            0x000b
#define LF_COBOL1_V1            0x000c
#define LF_BARRAY_V1            0x000d
#define LF_LABEL_V1             0x000e
#define LF_NULL_V1              0x000f
#define LF_NOTTRAN_V1           0x0010
#define LF_DIMARRAY_V1          0x0011
#define LF_VFTPATH_V1           0x0012
#define LF_PRECOMP_V1           0x0013
#define LF_ENDPRECOMP_V1        0x0014
#define LF_OEM_V1               0x0015
#define LF_TYPESERVER_V1        0x0016

#define LF_MODIFIER_V2          0x1001     /* variants with new 32-bit type indices (V2) */
#define LF_POINTER_V2           0x1002
#define LF_ARRAY_V2             0x1003
#define LF_CLASS_V2             0x1004
#define LF_STRUCTURE_V2         0x1005
#define LF_UNION_V2             0x1006
#define LF_ENUM_V2              0x1007
#define LF_PROCEDURE_V2         0x1008
#define LF_MFUNCTION_V2         0x1009
#define LF_COBOL0_V2            0x100a
#define LF_BARRAY_V2            0x100b
#define LF_DIMARRAY_V2          0x100c
#define LF_VFTPATH_V2           0x100d
#define LF_PRECOMP_V2           0x100e
#define LF_OEM_V2               0x100f

#define LF_SKIP_V1              0x0200
#define LF_ARGLIST_V1           0x0201
#define LF_DEFARG_V1            0x0202
#define LF_LIST_V1              0x0203
#define LF_FIELDLIST_V1         0x0204
#define LF_DERIVED_V1           0x0205
#define LF_BITFIELD_V1          0x0206
#define LF_METHODLIST_V1        0x0207
#define LF_DIMCONU_V1           0x0208
#define LF_DIMCONLU_V1          0x0209
#define LF_DIMVARU_V1           0x020a
#define LF_DIMVARLU_V1          0x020b
#define LF_REFSYM_V1            0x020c

#define LF_SKIP_V2              0x1200    /* variants with new 32-bit type indices (V2) */
#define LF_ARGLIST_V2           0x1201
#define LF_DEFARG_V2            0x1202
#define LF_FIELDLIST_V2         0x1203
#define LF_DERIVED_V2           0x1204
#define LF_BITFIELD_V2          0x1205
#define LF_METHODLIST_V2        0x1206
#define LF_DIMCONU_V2           0x1207
#define LF_DIMCONLU_V2          0x1208
#define LF_DIMVARU_V2           0x1209
#define LF_DIMVARLU_V2          0x120a

/* Field lists */
#define LF_BCLASS_V1            0x0400
#define LF_VBCLASS_V1           0x0401
#define LF_IVBCLASS_V1          0x0402
#define LF_ENUMERATE_V1         0x0403
#define LF_FRIENDFCN_V1         0x0404
#define LF_INDEX_V1             0x0405
#define LF_MEMBER_V1            0x0406
#define LF_STMEMBER_V1          0x0407
#define LF_METHOD_V1            0x0408
#define LF_NESTTYPE_V1          0x0409
#define LF_VFUNCTAB_V1          0x040a
#define LF_FRIENDCLS_V1         0x040b
#define LF_ONEMETHOD_V1         0x040c
#define LF_VFUNCOFF_V1          0x040d
#define LF_NESTTYPEEX_V1        0x040e
#define LF_MEMBERMODIFY_V1      0x040f

#define LF_BCLASS_V2            0x1400    /* variants with new 32-bit type indices (V2) */
#define LF_VBCLASS_V2           0x1401
#define LF_IVBCLASS_V2          0x1402
#define LF_FRIENDFCN_V2         0x1403
#define LF_INDEX_V2             0x1404
#define LF_MEMBER_V2            0x1405
#define LF_STMEMBER_V2          0x1406
#define LF_METHOD_V2            0x1407
#define LF_NESTTYPE_V2          0x1408
#define LF_VFUNCTAB_V2          0x1409
#define LF_FRIENDCLS_V2         0x140a
#define LF_ONEMETHOD_V2         0x140b
#define LF_VFUNCOFF_V2          0x140c
#define LF_NESTTYPEEX_V2        0x140d

#define LF_ENUMERATE_V3         0x1502
#define LF_ARRAY_V3             0x1503
#define LF_CLASS_V3             0x1504
#define LF_STRUCTURE_V3         0x1505
#define LF_UNION_V3             0x1506
#define LF_ENUM_V3              0x1507
#define LF_MEMBER_V3            0x150d

#define LF_NUMERIC              0x8000    /* numeric leaf types */
#define LF_CHAR                 0x8000
#define LF_SHORT                0x8001
#define LF_USHORT               0x8002
#define LF_LONG                 0x8003
#define LF_ULONG                0x8004
#define LF_REAL32               0x8005
#define LF_REAL64               0x8006
#define LF_REAL80               0x8007
#define LF_REAL128              0x8008
#define LF_QUADWORD             0x8009
#define LF_UQUADWORD            0x800a
#define LF_REAL48               0x800b
#define LF_COMPLEX32            0x800c
#define LF_COMPLEX64            0x800d
#define LF_COMPLEX80            0x800e
#define LF_COMPLEX128           0x800f
#define LF_VARSTRING            0x8010

/* ======================================== *
 *            Symbol information
 * ======================================== */

union codeview_symbol
{
    struct
    {
        short int	        len;
        short int	        id;
    } generic;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        offset;
	unsigned short	        segment;
	unsigned short	        symtype;
        struct p_string         p_name;
    } data_v1;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        symtype;
	unsigned int	        offset;
	unsigned short	        segment;
        struct p_string         p_name;
    } data_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            symtype;
        unsigned int            offset;
        unsigned short          segment;
        char                    name[1];
    } data_v3;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        pparent;
	unsigned int	        pend;
	unsigned int	        next;
	unsigned int	        offset;
	unsigned short	        segment;
	unsigned short	        thunk_len;
	unsigned char	        thtype;
        struct p_string         p_name;
    } thunk_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            pparent;
        unsigned int            pend;
        unsigned int            next;
        unsigned int            offset;
        unsigned short          segment;
        unsigned short          thunk_len;
        unsigned char           thtype;
        char                    name[1];
    } thunk_v3;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        pparent;
	unsigned int	        pend;
	unsigned int	        next;
	unsigned int	        proc_len;
	unsigned int	        debug_start;
	unsigned int	        debug_end;
	unsigned int	        offset;
	unsigned short	        segment;
	unsigned short	        proctype;
	unsigned char	        flags;
        struct p_string         p_name;
    } proc_v1;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        pparent;
	unsigned int	        pend;
	unsigned int	        next;
	unsigned int	        proc_len;
	unsigned int	        debug_start;
	unsigned int	        debug_end;
	unsigned int	        proctype;
	unsigned int	        offset;
	unsigned short	        segment;
	unsigned char	        flags;
        struct p_string         p_name;
    } proc_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            pparent;
        unsigned int            pend;
        unsigned int            next;
        unsigned int            proc_len;
        unsigned int            debug_start;
        unsigned int            debug_end;
        unsigned int            proctype;
        unsigned int            offset;
        unsigned short          segment;
        unsigned char           flags;
        char                    name[1];
    } proc_v3;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_BPREL_V1 */
	unsigned int	        offset;	        /* Stack offset relative to BP */
	unsigned short	        symtype;
        struct p_string         p_name;
    } stack_v1;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_BPREL_V2 */
	unsigned int	        offset;	        /* Stack offset relative to EBP */
	unsigned int	        symtype;
        struct p_string         p_name;
    } stack_v2;

    struct
    {
        short int               len;            /* Total length of this entry */
        short int               id;             /* Always S_BPREL_V3 */
        int                     offset;         /* Stack offset relative to BP */
        unsigned int            symtype;
        char                    name[1];
    } stack_v3;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_REGISTER */
        unsigned short          type;
        unsigned short          reg;
        struct p_string         p_name;
        /* don't handle register tracking */
    } register_v1;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_REGISTER_V2 */
        unsigned int            type;           /* check whether type & reg are correct */
        unsigned int            reg;
        struct p_string         p_name;
        /* don't handle register tracking */
    } register_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            parent;
        unsigned int            end;
        unsigned int            length;
        unsigned int            offset;
        unsigned short          segment;
        struct p_string         p_name;
    } block_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            parent;
        unsigned int            end;
        unsigned int            length;
        unsigned int            offset;
        unsigned short          segment;
        char                    name[1];
    } block_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            offset;
        unsigned short          segment;
        unsigned char           flags;
        struct p_string         p_name;
    } label_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            offset;
        unsigned short          segment;
        unsigned char           flags;
        char                    name[1];
    } label_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned short          type;
        unsigned short          cvalue;         /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } constant_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned                type;
        unsigned short          cvalue;         /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } constant_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned                type;
        unsigned short          cvalue;
#if 0
        char                    name[1];
#endif
    } constant_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned short          type;
        struct p_string         p_name;
    } udt_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned                type;
        struct p_string         p_name;
    } udt_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            type;
        char                    name[1];
    } udt_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            unknown;
        struct p_string         p_name;
    } compiland_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned                unknown1[4];
        unsigned short          unknown2;
        struct p_string         p_name;
    } compiland_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            unknown;
        char                    name[1];
    } compiland_v3;
};

#define S_COMPILAND_V1  0x0001
#define S_REGISTER_V1   0x0002
#define S_CONSTANT_V1   0x0003
#define S_UDT_V1        0x0004
#define S_SSEARCH_V1    0x0005
#define S_END_V1        0x0006
#define S_SKIP_V1       0x0007
#define S_CVRESERVE_V1  0x0008
#define S_OBJNAME_V1    0x0009
#define S_ENDARG_V1     0x000a
#define S_COBOLUDT_V1   0x000b
#define S_MANYREG_V1    0x000c
#define S_RETURN_V1     0x000d
#define S_ENTRYTHIS_V1  0x000e

#define S_BPREL_V1      0x0200
#define S_LDATA_V1      0x0201
#define S_GDATA_V1      0x0202
#define S_PUB_V1        0x0203
#define S_LPROC_V1      0x0204
#define S_GPROC_V1      0x0205
#define S_THUNK_V1      0x0206
#define S_BLOCK_V1      0x0207
#define S_WITH_V1       0x0208
#define S_LABEL_V1      0x0209
#define S_CEXMODEL_V1   0x020a
#define S_VFTPATH_V1    0x020b
#define S_REGREL_V1     0x020c
#define S_LTHREAD_V1    0x020d
#define S_GTHREAD_V1    0x020e

#define S_PROCREF_V1    0x0400
#define S_DATAREF_V1    0x0401
#define S_ALIGN_V1      0x0402
#define S_LPROCREF_V1   0x0403

#define S_REGISTER_V2   0x1001 /* Variants with new 32-bit type indices */
#define S_CONSTANT_V2   0x1002
#define S_UDT_V2        0x1003
#define S_COBOLUDT_V2   0x1004
#define S_MANYREG_V2    0x1005
#define S_BPREL_V2      0x1006
#define S_LDATA_V2      0x1007
#define S_GDATA_V2      0x1008
#define S_PUB_V2        0x1009
#define S_LPROC_V2      0x100a
#define S_GPROC_V2      0x100b
#define S_VFTTABLE_V2   0x100c
#define S_REGREL_V2     0x100d
#define S_LTHREAD_V2    0x100e
#define S_GTHREAD_V2    0x100f
#if 0
#define S_XXXXXXXXX_32  0x1012  /* seems linked to a function, content unknown */
#endif
#define S_COMPILAND_V2  0x1013

#define S_COMPILAND_V3  0x1101
#define S_THUNK_V3      0x1102
#define S_BLOCK_V3      0x1103
#define S_LABEL_V3      0x1105
#define S_CONSTANT_V3   0x1107
#define S_UDT_V3        0x1108
#define S_BPREL_V3      0x110B
#define S_LDATA_V3      0x110C
#define S_GDATA_V3      0x110D
#define S_PUB_DATA_V3   0x110E
#define S_LPROC_V3      0x110F
#define S_GPROC_V3      0x1110
#define S_MSTOOL_V3     0x1116  /* not really understood */
#define S_PUB_FUNC1_V3  0x1125  /* didn't get the difference between the two */
#define S_PUB_FUNC2_V3  0x1127

/* ======================================== *
 *          Line number information
 * ======================================== */

union any_size
{
    const char*                 c;
    const short*                s;
    const int*                  i;
    const unsigned int*         ui;
};

struct startend
{
    unsigned int	        start;
    unsigned int	        end;
};

struct codeview_linetab
{
    unsigned int		nline;
    unsigned int		segno;
    unsigned int		start;
    unsigned int		end;
    struct symt_compiland*      compiland;
    const unsigned short*       linetab;
    const unsigned int*         offtab;
};


/* ======================================== *
 *            PDB file information
 * ======================================== */


struct PDB_FILE
{
    DWORD               size;
    DWORD               unknown;
};

struct PDB_JG_HEADER
{
    CHAR                ident[40];
    DWORD               signature;
    DWORD               block_size;
    WORD                free_list;
    WORD                total_alloc;
    struct PDB_FILE     toc;
    WORD                toc_block[1];
};

struct PDB_DS_HEADER
{
    char                signature[32];
    DWORD               block_size;
    DWORD               unknown1;
    DWORD               num_pages;
    DWORD               toc_size;
    DWORD               unknown2;
    DWORD               toc_page;
};

struct PDB_JG_TOC
{
    DWORD               num_files;
    struct PDB_FILE     file[1];
};

struct PDB_DS_TOC
{
    DWORD               num_files;
    DWORD               file_size[1];
};

struct PDB_JG_ROOT
{
    DWORD               Version;
    DWORD               TimeDateStamp;
    DWORD               unknown;
    DWORD               cbNames;
    CHAR                names[1];
};

struct PDB_DS_ROOT
{
    DWORD               Version;
    DWORD               TimeDateStamp;
    DWORD               unknown;
    GUID                guid;
    DWORD               cbNames;
    CHAR                names[1];
};

typedef struct _PDB_TYPES_OLD
{
    DWORD       version;
    WORD        first_index;
    WORD        last_index;
    DWORD       type_size;
    WORD        file;
    WORD        pad;
} PDB_TYPES_OLD, *PPDB_TYPES_OLD;

typedef struct _PDB_TYPES
{
    DWORD       version;
    DWORD       type_offset;
    DWORD       first_index;
    DWORD       last_index;
    DWORD       type_size;
    WORD        file;
    WORD        pad;
    DWORD       hash_size;
    DWORD       hash_base;
    DWORD       hash_offset;
    DWORD       hash_len;
    DWORD       search_offset;
    DWORD       search_len;
    DWORD       unknown_offset;
    DWORD       unknown_len;
} PDB_TYPES, *PPDB_TYPES;

typedef struct _PDB_SYMBOL_RANGE
{
    WORD        segment;
    WORD        pad1;
    DWORD       offset;
    DWORD       size;
    DWORD       characteristics;
    WORD        index;
    WORD        pad2;
} PDB_SYMBOL_RANGE, *PPDB_SYMBOL_RANGE;

typedef struct _PDB_SYMBOL_RANGE_EX
{
    WORD        segment;
    WORD        pad1;
    DWORD       offset;
    DWORD       size;
    DWORD       characteristics;
    WORD        index;
    WORD        pad2;
    DWORD       timestamp;
    DWORD       unknown;
} PDB_SYMBOL_RANGE_EX, *PPDB_SYMBOL_RANGE_EX;

typedef struct _PDB_SYMBOL_FILE
{
    DWORD       unknown1;
    PDB_SYMBOL_RANGE range;
    WORD        flag;
    WORD        file;
    DWORD       symbol_size;
    DWORD       lineno_size;
    DWORD       unknown2;
    DWORD       nSrcFiles;
    DWORD       attribute;
    CHAR        filename[1];
} PDB_SYMBOL_FILE, *PPDB_SYMBOL_FILE;

typedef struct _PDB_SYMBOL_FILE_EX
{
    DWORD       unknown1;
    PDB_SYMBOL_RANGE_EX range;
    WORD        flag;
    WORD        file;
    DWORD       symbol_size;
    DWORD       lineno_size;
    DWORD       unknown2;
    DWORD       nSrcFiles;
    DWORD       attribute;
    DWORD       reserved[2];
    CHAR        filename[1];
} PDB_SYMBOL_FILE_EX, *PPDB_SYMBOL_FILE_EX;

typedef struct _PDB_SYMBOL_SOURCE
{
    WORD        nModules;
    WORD        nSrcFiles;
    WORD        table[1];
} PDB_SYMBOL_SOURCE, *PPDB_SYMBOL_SOURCE;

typedef struct _PDB_SYMBOL_IMPORT
{
    DWORD       unknown1;
    DWORD       unknown2;
    DWORD       TimeDateStamp;
    DWORD       nRequests;
    CHAR        filename[1];
} PDB_SYMBOL_IMPORT, *PPDB_SYMBOL_IMPORT;

typedef struct _PDB_SYMBOLS_OLD
{
    WORD        hash1_file;
    WORD        hash2_file;
    WORD        gsym_file;
    WORD        pad;
    DWORD       module_size;
    DWORD       offset_size;
    DWORD       hash_size;
    DWORD       srcmodule_size;
} PDB_SYMBOLS_OLD, *PPDB_SYMBOLS_OLD;

typedef struct _PDB_SYMBOLS
{
    DWORD       signature;
    DWORD       version;
    DWORD       unknown;
    DWORD       hash1_file;
    DWORD       hash2_file;
    DWORD       gsym_file;
    DWORD       module_size;
    DWORD       offset_size;
    DWORD       hash_size;
    DWORD       srcmodule_size;
    DWORD       pdbimport_size;
    DWORD       resvd[5];
} PDB_SYMBOLS, *PPDB_SYMBOLS;

#include "poppack.h"

/* ----------------------------------------------
 * Information used for parsing
 * ---------------------------------------------- */

typedef struct
{
    DWORD  from;
    DWORD  to;
} OMAP_DATA;

struct msc_debug_info
{
    struct module*              module;
    int			        nsect;
    const IMAGE_SECTION_HEADER* sectp;
    int			        nomap;
    const OMAP_DATA*            omapp;
    const BYTE*                 root;
};

/* coff.c */
extern BOOL coff_process_info(const struct msc_debug_info* msc_dbg);
