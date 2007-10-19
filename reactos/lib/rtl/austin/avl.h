/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/austin/avl.h
 * PURPOSE:         Run-Time Libary Header (interface to austin AVL tree)
 * PROGRAMMER:      arty
 */

#ifndef _REACTOS_RTL_LIB_AUSTIN_AVL_H
#define _REACTOS_RTL_LIB_AUSTIN_AVL_H

#define avl_data(x) ((void*)(&(x)[1]))

void avl_init(PRTL_AVL_TABLE table);
void avl_deinit(PRTL_AVL_TABLE table);
void avl_insert_node(PRTL_AVL_TABLE table, PRTL_BALANCED_LINKS node);
void avl_delete_node(PRTL_AVL_TABLE table, PRTL_BALANCED_LINKS node);
int  avl_is_nil(PRTL_AVL_TABLE table, PRTL_BALANCED_LINKS node);
PRTL_BALANCED_LINKS avl_first(PRTL_AVL_TABLE table);
PRTL_BALANCED_LINKS avl_last(PRTL_AVL_TABLE table);
PRTL_BALANCED_LINKS avl_next(PRTL_AVL_TABLE table, PRTL_BALANCED_LINKS node);

int  avl_search
(PRTL_AVL_TABLE table,
 PVOID _key,
 PRTL_BALANCED_LINKS node,
 PRTL_BALANCED_LINKS *where);


#endif/*_REACTOS_RTL_LIB_AUSTIN_AVL_H*/
