//=======================================================================
//		File:	FOCUSDEF.H
//		Date: 	1-20-98
//		Desc:	Contains definitions of related to the focus.
//=======================================================================

#ifndef __FOCUSDEFINITIONS__
#define __FOCUSDEFINITIONS__


//================================================================================
// Defines
//================================================================================

    //----------------------------------------------
    // Default setting for current focused object.
    //
    //	When we receive a MSAA STATECHANGE event
    //	from Trident, we the document's current
    //	focused object ID to this value to indicate
    //	that, as far as we are concerned, no object
    //	has the focus.
    //
    //	This constant is used in prxymgr.cpp and 
    //	document.cpp.
    //----------------------------------------------

#define NO_TRIDENT_FOCUSED      (-1)


    //----------------------------------------------
    // Default setting for AOM Manager's current
    //	focused ID.
    //
    //	This constant must be different from
    //	NO_TRIDENT_FOCUSED because it is possible
    //	that the AOM Manager methods dealing with
    //	focused object resolution could be called
    //	with this value.
    //	
    //	This constant is used in aommgr.h/.cpp.
    //----------------------------------------------

#define INOOBJECTFOCUS          (NO_TRIDENT_FOCUSED - 1)


    //----------------------------------------------
    // Manifest constant used to indicate that no
    //  object in the given frame has the focus,
    //  but that the browser itself is the active
    //  window so all objects that can be given the
    //  focus should have a state including
    //  STATE_SYSTEM_FOCUSABLE.
    //
    // This constant is used as an UINT.
    //----------------------------------------------

#define ALL_OBJECTS_FOCUSABLE   (INOOBJECTFOCUS - 1)
#define ALL_OBJECTS_SELECTABLE  (ALL_OBJECTS_FOCUSABLE)



#endif  //__FOCUSDEFINITIONS__