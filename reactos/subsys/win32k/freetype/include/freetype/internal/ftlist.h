/***************************************************************************/
/*                                                                         */
/*  ftlist.c                                                               */
/*                                                                         */
/*    Generic list support for FreeType (specification).                   */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /*  This file implements functions relative to list processing.  Its     */
  /*  data structures are defined in `freetype.h'.                         */
  /*                                                                       */
  /*************************************************************************/


#ifndef FTLIST_H
#define FTLIST_H

#include <freetype/freetype.h>

#ifdef __cplusplus
  extern "C" {
#endif


  FT_EXPORT_DEF( FT_ListNode )  FT_List_Find( FT_List  list,
                                              void*    data );

  FT_EXPORT_DEF( void )  FT_List_Add( FT_List      list,
                                      FT_ListNode  node );

  FT_EXPORT_DEF( void )  FT_List_Insert( FT_List      list,
                                         FT_ListNode  node );

  FT_EXPORT_DEF( void )  FT_List_Remove( FT_List      list,
                                         FT_ListNode  node );

  FT_EXPORT_DEF( void )  FT_List_Up( FT_List      list,
                                     FT_ListNode  node );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FT_List_Iterator                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An FT_List iterator function which is called during a list parse   */
  /*    by FT_List_Iterate().                                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    node :: The current iteration list node.                           */
  /*                                                                       */
  /*    user :: A typeless pointer passed to FT_List_Iterate().            */
  /*            Can be used to point to the iteration's state.             */
  /*                                                                       */
  typedef FT_Error  (*FT_List_Iterator)( FT_ListNode  node,
                                         void*        user );


  FT_EXPORT_DEF( FT_Error )  FT_List_Iterate( FT_List           list,
                                              FT_List_Iterator  iterator,
                                              void*             user );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FT_List_Destructor                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An FT_List iterator function which is called during a list         */
  /*    finalization by FT_List_Finalize() to destroy all elements in a    */
  /*    given list.                                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    system :: The current system object.                               */
  /*                                                                       */
  /*    data   :: The current object to destroy.                           */
  /*                                                                       */
  /*    user   :: A typeless pointer passed to FT_List_Iterate().  It can  */
  /*              be used to point to the iteration's state.               */
  /*                                                                       */
  typedef void  (*FT_List_Destructor)( FT_Memory  memory,
                                       void*      data,
                                       void*      user );


  FT_EXPORT_DEF( void )  FT_List_Finalize( FT_List             list,
                                           FT_List_Destructor  destroy,
                                           FT_Memory           memory,
                                           void*               user );


#ifdef __cplusplus
  }
#endif

#endif /* FTLIST_H */


/* END */
