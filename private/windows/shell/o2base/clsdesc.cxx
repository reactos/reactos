//+---------------------------------------------------------------------
//
//   File:       clsdesc.cxx
//
//   Contents:   ClassDescriptor implementation
//
//------------------------------------------------------------------------

//[ classdescriptor_overview
/*
                        ClassDescriptor Overview

A ClassDescriptor is a structure the contains global, static information
about an OLE Compound Document class.  Having a ClassDescriptor allows
the base classes to do a lot of work on behalf of the derived class because
it can get information it needs from the ClassDescriptor instead of resorting
to virtual method calls.  A single ClassDescriptor is shared by all object
instances of that Compound Document class.

A ClassDescriptor has three conceptual parts.  The first part is a collection
of information loaded from resources.  This includes strings, menus, accelerators,
and an icon.  The second part is a pair of verb tables that are used to implement
IOleObject::DoVerb.  The third part is two pairs of format tables that are
used to implement many of the Get/Set/Query methods on IDataObject.
Each of these parts has its own initialization method to prepare that part of
the class descriptor.

The ClassDescriptor is usually associated with the ClassFactory.

*/
//]

#include "headers.hxx"
#pragma hdrstop

#define MAX_USERTYPE_LEN 40

//+---------------------------------------------------------------
//
//  Member:     ClassDescriptor::ClassDescriptor
//
//  Synopsis:   Constructor for ClassDescriptor structure
//
//  Notes:      A DLL (in-process) server should allocate the class
//              descriptor at library initialization time and allocate
//              it in shared memory (GMEM_SHARE flag with GlobalAlloc, or
//              the OLE shared allocator, MEMCTX_SHARED).
//
//              The constructor ensures all members of the structure
//              are initialized to NULL
//
//----------------------------------------------------------------

ClassDescriptor::ClassDescriptor(void)
{
    // initialize everything to NULL
    memset(this, 0, sizeof(ClassDescriptor));
}

//+---------------------------------------------------------------
//
//  Member:     ClassDescriptor::Init
//
//  Synopsis:   Initializes a class descriptor structure from resources
//
//  Arguments:  [hinst] -- instance handle of module with resources used
//                      to initialize the class descriptor
//
//  Returns:    TRUE iff the class descriptor was successfully initialized
//
//  Notes:      This method only fills in the part of the class descriptor
//              that can be initialized from resources.  The other part
//              that requires initialization are the verb and format tables.
//
//              Objects that are concerned about resource load times could
//              not use this initialization method and fill in fields of the
//              class descriptor directly.
//
//----------------------------------------------------------------

BOOL ClassDescriptor::Init(HINSTANCE hinst, WORD wBaseID)
{
    //REVIEW:  Currently, information loaded from resources is duplicated
    //REVIEW:  in the registration database.  We could
    //REVIEW:   (1) load this stuff from the reg db instead (except CLSID), or
    //REVIEW:   (2) do an autoregistration feature where the reg db is filled
    //REVIEW:       in from the class descriptor information loaded from resources

    //REVIEW:  We could also support loading the verb and format tables
    //REVIEW:  from resources/regdb as well.

    _hinst = hinst;
    _wBaseResID = wBaseID;

    // load our class id string and convert it into our real class id
    WCHAR szClsId[40];
    LoadString(hinst, wBaseID+IDOFF_CLASSID, szClsId, sizeof(szClsId));
    BOOL fRet = OK(CLSIDFromString(szClsId, &_clsid));

    // load our in-place menus and accelerators
    _hicon = LoadIcon(hinst, MAKEINTRESOURCE(wBaseID+IDOFF_ICON));
    _haccel = (HACCEL)LoadAccelerators(hinst,
            MAKEINTRESOURCE(wBaseID+IDOFF_ACCELS));

    LoadResourceData(hinst,
            MAKEINTRESOURCE(wBaseID+IDOFF_MGW),
            &_mgw,
            sizeof(_mgw));

    LoadResourceData(hinst,
            MAKEINTRESOURCE(wBaseID+IDOFF_MISCSTATUS),
            &_dwMiscStatus,
            sizeof(_dwMiscStatus));

    LoadString(hinst,
            wBaseID+IDOFF_USERTYPEFULL,
            _szUserClassType[USERCLASSTYPE_FULL],
            MAX_USERTYPE_LEN+sizeof(WCHAR));

    LoadString(hinst,
            wBaseID+IDOFF_USERTYPESHORT,
            _szUserClassType[USERCLASSTYPE_SHORT],
            MAX_USERTYPE_LEN+sizeof(WCHAR));

    LoadString(hinst,
            wBaseID+IDOFF_USERTYPEAPP,
            _szUserClassType[USERCLASSTYPE_APPNAME],
            MAX_USERTYPE_LEN+sizeof(WCHAR));

    LoadString(hinst,
            wBaseID+IDOFF_DOCFEXT,
            _szDocfileExt,
            sizeof(_szDocfileExt));

    return fRet;
}

//+---------------------------------------------------------------
//
//  Member:     ClassDescriptor::LoadMenu, public
//
//  Synopsis:   Loads a copy of the menus for the server
//
//  Notes:      A single copy of the menu cannot be shared by all
//              instances of the class, like an accelerator table can.
//              This is because the menu is not a read-only resource
//              (e.g. you can "check" a menu item and merge in verb
//              menu items).  Therefore each class instance must load
//              its own copy of the menu.  This LoadMenu call is used
//              for that purpose.
//
//---------------------------------------------------------------

HMENU
ClassDescriptor::LoadMenu(void)
{
    return ::LoadMenu(_hinst, MAKEINTRESOURCE(_wBaseResID+IDOFF_MENU));
}

