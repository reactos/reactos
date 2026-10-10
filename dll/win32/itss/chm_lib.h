/***************************************************************************
 *             chm_lib.h - CHM archive manipulation routines               *
 *                           -------------------                           *
 *                                                                         *
 *  author:     Jed Wing <jedwin@ugcs.caltech.edu>                         *
 *  version:    0.3                                                        *
 *  notes:      These routines are meant for the manipulation of microsoft *
 *              .chm (compiled html help) files, but may likely be used    *
 *              for the manipulation of any ITSS archive, if ever ITSS     *
 *              archives are used for any other purpose.                   *
 *                                                                         *
 *              Note also that the section names are statically handled.   *
 *              To be entirely correct, the section names should be read   *
 *              from the section names meta-file, and then the various     *
 *              content sections and the "transforms" to apply to the data *
 *              they contain should be inferred from the section name and  *
 *              the meta-files referenced using that name; however, all of *
 *              the files I've been able to get my hands on appear to have *
 *              only two sections: Uncompressed and MSCompressed.          *
 *              Additionally, the ITSS.DLL file included with Windows does *
 *              not appear to handle any different transforms than the     *
 *              simple LZX-transform.  Furthermore, the list of transforms *
 *              to apply is broken, in that only half the required space   *
 *              is allocated for the list.  (It appears as though the      *
 *              space is allocated for ASCII strings, but the strings are  *
 *              written as unicode.  As a result, only the first half of   *
 *              the string appears.)  So this is probably not too big of   *
 *              a deal, at least until CHM v4 (MS .lit files), which also  *
 *              incorporate encryption, of some description.               *
 ***************************************************************************/

/***************************************************************************
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 ***************************************************************************/

#ifndef INCLUDED_CHMLIB_H
#define INCLUDED_CHMLIB_H

typedef ULONGLONG LONGUINT64;
typedef LONGLONG  LONGINT64;

/* the two available spaces in a CHM file                      */
/* N.B.: The format supports arbitrarily many spaces, but only */
/*       two appear to be used at present.                     */
#define CHM_UNCOMPRESSED (0)
#define CHM_COMPRESSED   (1)

/* structure representing an ITS (CHM) file stream             */
struct chmFile;

/* structure representing an element from an ITS file stream   */
#define CHM_MAX_PATHLEN  (256)
struct chmUnitInfo
{
    LONGUINT64         start;
    LONGUINT64         length;
    int                space;
    WCHAR              path[CHM_MAX_PATHLEN+1];
};

struct chmFile* chm_openW(const WCHAR *filename);
struct chmFile *chm_dup(struct chmFile *oldHandle);

/* close an ITS archive */
void chm_close(struct chmFile *h);

/* resolve a particular object from the archive */
#define CHM_RESOLVE_SUCCESS (0)
#define CHM_RESOLVE_FAILURE (1)
int chm_resolve_object(struct chmFile *h, const WCHAR *objPath, struct chmUnitInfo *ui);

/* retrieve part of an object from the archive */
LONGINT64 chm_retrieve_object(struct chmFile *h, struct chmUnitInfo *ui, unsigned char *buf,
                              LONGUINT64 addr, LONGINT64 len);

/* enumerate the objects in the .chm archive */
typedef int (*CHM_ENUMERATOR)(struct chmFile *h,
                              struct chmUnitInfo *ui,
                              void *context);
#define CHM_ENUMERATE_NORMAL    (1)
#define CHM_ENUMERATE_META      (2)
#define CHM_ENUMERATE_SPECIAL   (4)
#define CHM_ENUMERATE_FILES     (8)
#define CHM_ENUMERATE_DIRS      (16)
#define CHM_ENUMERATE_ALL       (31)
#define CHM_ENUMERATOR_FAILURE  (0)
#define CHM_ENUMERATOR_CONTINUE (1)
#define CHM_ENUMERATOR_SUCCESS  (2)
BOOL chm_enumerate_dir(struct chmFile *h, const WCHAR *prefix, int what,
                       CHM_ENUMERATOR e, void *context);

#endif /* INCLUDED_CHMLIB_H */
