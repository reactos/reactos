/*****************************************************************************
*                                                                            *
*  LL.H                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description: Exports link list functions                          *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:                                                         *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*                                                                            *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define nilHAND  (HANDLE)NULL
#define nilLL   nilHAND
#define nilHLLN nilHAND


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef HANDLE  LL;
typedef HANDLE  HLLN;


/*******************
**
** Name:       LLCreate
**
** Purpose:    Creates a link list
**
** Arguments:  None.
**
** Returns:    Link list.  nilLL is returned if an error occurred.
**
*******************/

LL FAR APIENTRY LLCreate(VOID);


/*******************
**
** Name:       InsertLL
**
** Purpose:    Inserts a new node at the head of the linked list
**
** Arguments:  ll     - link list
**             vpData - pointer to data to be associated with
**             c      - count of the bytes pointed to by vpData
**
** Returns:    TRUE iff insertion is successful.
**
*******************/

BOOL FAR APIENTRY InsertLL(LL, VOID FAR *, LONG);


/*******************
**
** Name:       WalkLL
**
** Purpose:    Mechanism for walking the nodes in the linked list
**
** Arguments:  ll   - linked list
**             hlln - handle to a linked list node
**
** Returns:    a handle to a link list node or NIL_HLLN if at the
**             end of the list (or an error).
**
** Notes:      To get the first node, pass NIL_HLLN as the hlln - further
**             calls should use the HLLN returned by this function.
**
*******************/

HLLN FAR APIENTRY WalkLL(LL, HLLN);


/*******************
**
** Name:       QVLockHLLN
**
** Purpose:    Locks a LL node returning a pointer to the data
**
** Arguments:  hlln - handle to a linked list node
**
** Returns:    pointer to data or NULL if an error occurred
**
*******************/

VOID FAR * FAR QVLockHLLN(HLLN);


/*******************
**
** Name:       QVUnlockHLLN
**
** Purpose:    Unlocks a LL node
**
** Arguments:  hlln - handle to a link list node
**
** Returns:    TRUE iff the handle is successfully locked.
**
*******************/

VOID FAR UnlockHLLN(HLLN);


/*******************
**
** Name:       DestroyLL
**
** Purpose:    Deletes a LL and all of its contents
**
** Arguments:  ll - linked list
**
** Returns:    Nothing.
**
*******************/

VOID FAR APIENTRY DestroyLL(LL);
