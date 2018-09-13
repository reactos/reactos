/*
 * link - The Rundll that turns off "Shortcut to"
 *
 * This works around a bug in Shell32, where an off-by-one prevented
 * the restore of the setting from working.
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

KL c_klLink = { &g_hkCUSMWCV, c_tszExplorer, c_tszLink };

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Link_GetShortcutTo
 *
 *  Determine whether the "Shortcut to" prefix is enabled.
 *
 *****************************************************************************/

BOOL PASCAL
Link_GetShortcutTo(void)
{
    return GetDwordPkl(&c_klLink, 1) > 0;
}

/*****************************************************************************
 *
 *  fCreateNil
 *
 *	Create a zero-length file.
 *
 *****************************************************************************/

BOOL PASCAL
fCreateNil(LPCTSTR cqn)
{
    HFILE hf = _lcreat(cqn, 0);
    if (hf != -1) {
	_lclose(hf);
	return 1;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  Link_Drop		-- Create a temp directory, then...
 *  Link_DropCqn	-- create a pidl for the directory, then...
 *  Link_DropPidlCqn	-- bind to the pidl, then...
 *  Link_DropPsfCqn	-- try 20 times to...
 *  Link_RenameToBang	-- rename a scratch pidl to "!"
 *
 *	(Welcome to lisp.)
 *
 *	Keep renaming a file, losing the "Shortcut to", until the shell
 *	finally gets the point, or we've tried 20 times and give up.
 *	If the shell doesn't get the point after 20 tries, it'll never
 *	learn...
 *
 *	Returns 0 if we couldn't do it.
 *
 *	We do this by creating a temporary directory within the temp
 *	directory.  In this temp-temp directory, create a file called
 *	"Shortcut to !.lnk", then keep renaming it to "!".
 *
 *	By creating it in a brand new temp dir, we are sure we won't
 *	conflict with any other files.
 *
 *****************************************************************************/

BOOL PASCAL
Link_RenameToBang(PIDL pidl, LPVOID pv)
{
    PSF psf = pv;
    DeleteFile(c_tszBangLnk);	/* So the rename will work */
    return SetNameOfPidl(pv, pidl, c_tszBang);
}

BOOL PASCAL
Link_DropPsfCqn(PSF psf, LPCTSTR cqn)
{
    if (fCreateNil(c_tszBang)) {
	BOOL fRc;
	TCH tszLinkToBang[MAX_PATH];
	if (mit.SHGetNewLinkInfo(c_tszBang, cqn, tszLinkToBang, &fRc,
				 SHGNLI_PREFIXNAME)) {
	    int iter;
	    for (iter = 0; iter < 20 && Link_GetShortcutTo(); iter++) {
		fCreateNil(tszLinkToBang);
		WithPidl(psf, ptszFilenameCqn(tszLinkToBang),
			 Link_RenameToBang, psf);
	    }
	}
    }
    return !Link_GetShortcutTo();
}

BOOL PASCAL
Link_DropPidlCqn(PIDL pidl, LPVOID cqn)
{
    return WithPsf(psfDesktop, pidl, Link_DropPsfCqn, cqn);
}


BOOL PASCAL
Link_DropCqn(LPCTSTR cqn, LPVOID pv)
{
    return WithPidl(psfDesktop, cqn, Link_DropPidlCqn, (LPVOID)cqn);
}

Link_Drop(void)
{
    return WithTempDirectory(Link_DropCqn, 0);
}

/*****************************************************************************
 *
 *  Link_SetShortcutTo
 *
 *	Set or clear the "prepend "Shortcut to" to new shortcuts" flag.
 *
 *	If we need to set it, then set the registry key and ask the user
 *	to log off and back on.  There is no way to make the count go up.
 *
 *	If we need to clear it, then keep renaming "Shortcut to frob" to
 *	"frob" until the link count goes to zero.
 *
 *	Returns 0 if the user must log off and back on for the change
 *	to take effect.
 *
 *****************************************************************************/

BOOL PASCAL
Link_SetShortcutTo(BOOL fPrefix)
{
    if (fPrefix != Link_GetShortcutTo()) {
	if (fPrefix) {
	    DelPkl(&c_klLink);
	    return 0;			/* Must log off and back on */
	} else {			/* Make the count drop to zero */
	    if (Link_Drop()) {
		return 1;
	    } else {
		SetDwordPkl(&c_klLink, fPrefix);
					/* Oh well */
		return 0;
	    }
	}
    } else {
	return 1;
    }
}

