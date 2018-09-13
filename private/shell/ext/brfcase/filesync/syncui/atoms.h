//
// atoms.h: Atom handling
//
//

#ifndef __ATOMS_H__
#define __ATOMS_H__


/////////////////////////////////////////////////////  DEFINES

// Atom Table 
//
// We have our own atoms for two reasons:
//
//  1) Gives us greater flexibility for partial string searches,
//      in-place string replacements, and table resize
//  2) We don't know yet if Windows' local atom tables are sharable
//      in separate instances in Win32.
//

BOOL    PUBLIC Atom_Init (void);
void    PUBLIC Atom_Term (void);
int     PUBLIC Atom_Add (LPCTSTR psz);
UINT    PUBLIC Atom_AddRef(int atom);
void    PUBLIC Atom_Delete (int atom);
BOOL    PUBLIC Atom_Replace (int atom, LPCTSTR pszNew);
int     PUBLIC Atom_Find (LPCTSTR psz);
LPCTSTR  PUBLIC Atom_GetName (int atom);
BOOL    PUBLIC Atom_IsPartialMatch(int atom1, int atom2);
BOOL    PUBLIC Atom_Translate(int atomOld, int atomNew);

#define Atom_IsChildOf(atom1, atom2)    Atom_IsPartialMatch(atom1, atom2)
#define Atom_IsParentOf(atom1, atom2)   Atom_IsPartialMatch(atom2, atom1)

#define ATOM_ERR    (-1)

#define Atom_IsValid(atom)      (ATOM_ERR != (atom) && 0 != (atom))

#ifdef DEBUG

void    PUBLIC Atom_ValidateFn(int atom);
void    PUBLIC Atom_DumpAll();

#define VALIDATE_ATOM(atom)     Atom_ValidateFn(atom)

#else  // DEBUG

#define VALIDATE_ATOM(atom)

#endif // DEBUG

#endif // __ATOMS_H__

