
//+----------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       txtparse.cxx
//
//  Contents:   CHtmTextContext class
//
//-----------------------------------------------------------------------------


#include "headers.hxx"

//+----------------------------------------------------------------------------
//
//  Function:   InPre
//
//  Synopsis:   This computes the pre status of a branch after an element goes
//              out of scope.  The branch before the element goes out of scope
//              is passed in, as well as the element going out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
InPre ( CTreeNode * pNodeCur, CTreeNode * pNodeEnd )
{
    CTreeNode *pNode;

    for ( pNode = pNodeCur ; pNode ; pNode = pNode->Parent() )
    {
        if (!pNodeEnd || DifferentScope( pNode, pNodeEnd ))
        {
            switch ( TagPreservationType( pNode->Tag()) )
            {
            case WSPT_PRESERVE : return pNode;
            case WSPT_COLLAPSE : return NULL;
            case WSPT_NEITHER  : break;
            default            : Assert( 0 );
            }
        }
    }

    return NULL;
}


