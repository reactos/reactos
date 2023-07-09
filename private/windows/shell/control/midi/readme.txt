MIDI Mapper Control Panel Applet        Mike McQueen (t-mikemc) 1-Oct-90
--------------------------------
                                        Hastily updated by LaurieGr

This document describes how the MIDI Mapper applet was put together, in
order to hopefully make it easier for someone other than myself to find
and fix any bugs or change some features.

Currently the source for this applet is divided into seven modules,
four header files and a resource script file.


Descrption of MIDI Mapper applet:
================================

Modules
-------
        
LIBINIT.ASM
        Contains the LibEntry function.  This piece of code sends shivers
        up my spine every time I look at it.  Someone (other than ToddLa)
        really ought to make a nice clean standard DLL entry point asm
        module.  Doesn't apply to NT.

MIDI.C
        Contains the LibMain and WEP functions for windows.

        Contains the CPlApplet export function for multimedia control panel.
        This is the entry point that the controlpanel calls.
        Start debugging here!

        Contains the 'MIDI Mapper' dialog box function.  Among other things,
        this dialog box has three combo boxes into which the available setups,
        patchmaps and keymaps are respectively enumerated.  When someone
        deletes a map, it's not actually deleted from the file, but rather
        its entry is simply removed from the combo box.  If the guy then
        clicks on the 'OK' button, interpreted as 'OK, go ahead and actually
        delete the maps', I compare the available maps to the maps left
        in the combo boxes, and delete any discrepancies.  This is done in
        the EnumFunc function described below.

        This module also contains several functions that are used by one or
        more of the map editing modules.  I probably should have thrown
        together a 'MISC.C' module and tossed all these functions in there,
        but in any case all these functions are at the bottom of the file.
        The WriteBoxRect and SizeBox functions were only used in the version
        of the applet that allowed sizing of map-editing boxes.  If sizing
        is never ever going to be used, these can be removed.

        **EnumFunc**

        This lovely little treat of a function is the callback for the
        mapEnumerate function from midimap.c in the MMSYSTEM DLL.  it is used
        by calls to mapEnumerate from 3 different modules, MIDI.C, SETUP.C
        and PATCH.C.
        mapEnumerate is called to enumerate Setups, Maps, Patches or ports
        which don't have an awful lot in common.  It's also called from
        all over the place.  The price for having only one copy of the 16
        statements that actually do the enumeration is that all the different
        callback functions are forced into the same straightjacket of
        parameters.  On 16 bit it inevitably had a handle packed into half
        a DWORD.  The callback which is executed once for each element now
        has three parameters to cater for all cases (with a little casting)
        and some subset of these are actually used in each case.

        The SETUP.C and PATCH.C modules simply call it to enumerate the
        available patchmaps and keymaps into their combo boxes, respectively.

        In MIDI.C, the HIWORD of the dwUser parameter is either set to
        MMENUM_DELETE, MMENUM_BASIC or  MMENUM_INTOCOMBO.

        If it is set to MMENUM_INTOCOMBO, it indicates that this enumeration
        is taking place into the combo box of the MIDI Mapper dialog box.
        When this happens, a handle is allocated for the length of the
        description string, and this handle is set to be the new combo box
        entry's ITEMDATA.  This is how I update the description when the user
        changes the current selection in the combo box in the main dialog.

        If it is set to MMENUM_DELETE, it indicates that a check is being done
        to see if any entrys in the combo box differ from the available maps
        being enumerated.  In this case, if a map name is sent to EnumFunc
        from mapEnumerate, and it does not exist in the combo box, it will be
        deleted right then and there with a call to mapDelete (another
        midimap.c function).
        
SETUP.C
PATCH.C
KEY.C
        These modules are pretty similar in content, so I am going to try to
        describe them all in one plop.  A lot of the functions from these
        three modules have the same name, but are prefixed with 'Setup',
        'Patch' or 'Key' depending on which module they are in. I'm going to
        use 'xxx' to mean any or all of those three prefixes.

        The 'xxxBox' functions are the three dialog box functions for the
        three map-editing dialog boxes.

        The 'xxxPaint' functions are the only places where anything is ever
        drawn on the screen.  These functions are essentially the BeginPaint
        EndPaint blocks for each map editor.

        In the setup editor, I do a funny little trick to allow tabbing onto
        three-state buttons, and it would probably be a good idea to explain
        it.  Every time SetupSetFocus is called, I remove the WS_TABSTOP style
        attribute from the 3-state button on the last edit line, and add it
        to the 3-state button on the new edit line.  I also change the text to
        "&A" which allows the ALT-A accelerator to work.

        Global variables which pertain to all three of these modules are
        described under the 'extern.h' header file.
        
PROPBOX.C
        Contains only a dialog box function for getting properties for a new
        map of any type.  If the 'fSaveAs' variable is set, it will
        automagically check for duplicate names.  Unfortunately, I don't have
        any way of doing a 'save as...' in the current version.  If you think
        of a way, you could just call this dialog box with the fSaveAs
        variable set.

CPARROW.C
        Contains a routine for registering a window class for a control panel
        arrow control, as well as the window function for that class.  There
        is a arrow in WINCOM, and there is another arrow in SCRNSVR (that's
        where I stole this code from).  I think my version works/looks better
        than both of the others.  I also think it still needs to be worked
        on.  I think that if and when this ever gets done, it should go in
        WINCOM and then the static link modules could go away.

Headers
-------

MIDI.H
        Contains all sorts of definitions, and I've tried to document them
        as best as possible.
        
PRECLUDE.H
        Contains 'preclude' definitions for windows.h and mmsystem.h, to
        make compiling a little faster.

CPARROW.H
        Contains the prototypes for the arrow class (un)registering functions.

EXTERN.H
        Contains external variable definitions, with limited comments.  I'll
        comment them again right here:

extern HWND     hWnd                    // 'Current' window handle
                hEdit,                  // Edit control handle
                hArrow;                 // Arrow control handle
extern RECT     rcBox;                  // Clipping/scroll rectangle
extern int      rgxPos [8],             // horizontal line positions
                yBox,                   // rows of data y extent
                xClient,                // Window client area x pixels
                yClient,                // Window client area y pixels
                iCurPos,                // Current position on screen
                iVertPos,               // Current vertical scroll position
                iVertMax,               // Maximum veritcal scroll position
                nLines,                 // Number of lines of data
                yChar,                  // Height of character in font
                xChar,                  // Width of average character in font
                iMap;                   // Flag for GetMBData
extern char     szCurrent [],           // Name of current map
                szCurDesc [],           // Description of current map
                szMidiCtl [],           // "MIDI Control Panel"
                szNone [];              // Static text string '[ None ]'
extern BOOL     fModified,              // Has map been modified?
                fNew,                   // Is this a new map?
                fSaveAs,                // Is propbox being used for Save As?
                fHidden;                // Is the active edit line hidden?

Resource script
---------------
        midi.rc


Problems with MIDI Mapper applet:
================================

Currently, the maximum number of any type of map (setup,patchmap,keymap) is
limited to 100.  This is a deficiency in midimap routines which are part of
mmsystem.  If someone calls the mapWrite function with the flag set to the
type of a map of which there are 100, the write will fail.  The only way to
currently get around this problem is deleting maps.

In the process of changing the current selection of either the "Keymap Name"
combobox in the Patchmap editor, or the "Patchmap Name" combobox in the Setup
editor, memory is potentially being reallocated without fail checks, which
means it should be possible to run out of memory in a bad way if the current
selection of these comboboxes is changed a lot.

For the patchmap editor, this is absolutely silly, since all keymaps are the
same size.  I should never have to reallocate the whole setup just so it turns
out to be the exact same size!  Anyway, the problem arises when you have a
patchmap with some keymaps.  Someone derefernces a keymap from a specific
patch number in the patchmap editor (that is, selects the [none] entry for
that particular patch).  This in turns causes the keymap offset of that
patchmap to be set to zero.  If the person proceeds to re-reference that
patchmap back to the same keymap, or any other keymap, the entire setup will
be reallocated, to the same size.  Although this will call will probably never
fail, it is unnecessary.

A flag lying around in the patchmap structure which indicates that the memory
for a keymap has been allocated make this very useful.  Perhaps it would be a
good idea to define a range of flags in the dwFlags structure entry of the
MIDIKEYMAP/MIDIPATCHMAP/MIDIMAP data structures which are for internal use
only, some reserved bits.

The formula for the size of the new setup is <old setupsize> + <new patchsize>,
which does not take into account subtracting <old patchsize>.  The reason for
this is that the patchmap may be referenced by other channels in the setup.
You cannot free the memory of the patchmap just because one channel doesn't
reference it anymore.  The thing is, it would be really easy to check and see
if anyother channel references it.  There are two APIs called:

mapPatchMapInSetup and..
mapKeyMapInPatchMap

which are exported from mmsystem.  These routines should be used by the

SetupComboMsg

function, under the CBN_SELCHANGE message.  It is here that the existence
of any given patchmap name in the current setup can be determined by calls
to the former API.  It is not necessary to reallocate an entire setup if
one channel in that setup suddenly references a patchmap that is already
referenced in that setup.  All that should happen is the new channel's
patchmap offset value should be set to that of the existing channel.  In a
similar instance, if a patchmap is dereferenced by a channel of a setup, and
it is determined that no other channels of that setup use that patchmap, the
size of that patchmap may be subtracted from the setup reallocation size.

