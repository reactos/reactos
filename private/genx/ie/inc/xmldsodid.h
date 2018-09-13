//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996-1997               **
//*********************************************************************

//;begin_internal
/***********************************************************************************************

  This is a distributed SDK component - do not put any #includes or other directives that rely
  upon files not dropped. If in doubt - build iedev

  If you add comments please include either ;BUGBUG at the beginning of a single line OR
  enclose in a ;begin_internal, ;end_internal block - such as this one!

 ***********************************************************************************************/
//;end_internal

//;begin_internal
#ifndef __XMLDSODID_H__
#define __XMLDSODID_H__
//;end_internal

#define DISPID_XOBJ_MIN                 0x00010000
#define DISPID_XOBJ_MAX                 0x0001FFFF
#define DISPID_XOBJ_BASE                DISPID_XOBJ_MIN

#define  DISPID_XMLDSO                       DISPID_XOBJ_BASE
#define  DISPID_XMLDSO_DOCUMENT              DISPID_XMLDSO  +  1
#define  DISPID_XMLDSO_JAVADSOCOMPATIBLE     DISPID_XMLDSO_DOCUMENT  +  1

//;begin_internal
#endif // __XMLDSODID_H__
//;end_internal
