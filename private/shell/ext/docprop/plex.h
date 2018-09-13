/*
 *      plex.h
 *
 *      Structures and definitions for using plexs.
 */

#ifndef __plex__
#define __plex__

#define WAlign(w) (((w)+1)&~1)
/*      comparison return values
 */
#define sgnGT   1
#define sgnEQ   0
#define sgnLT   (-1)
#define sgnNE  2

/*----------------------------------------------------------------------------
|       PL structure
|
|       The PL (pronounced "plex") structure is used to efficiently
|       manipulate variable sized arrays.
|
|       Fields:
|               iMax            number of allocated items
|               fUseCount use count plex (not all plex API's work w/ UseCount plexes)
|               iMac            last used allocated item
|               cbItem          sizeof item
|               dAlloc          number of items to allocate at a time
|               dgShift         data group to the plex should be
|                               allocated in
|               rg              the array of items
|
| WARNING: this structure also in winpm\excel.inc & mac\excel.i
|
----------------------------------------------------------------------------*/
typedef struct _pl
                {
                WORD    iMax : 15,
                                fUseCount : 1;
                SHORT   iMac;
                WORD    cbItem;
                WORD    dAlloc:5,
                                unused:11;
                BYTE    rg[1];
                }
        PL;

/*----------------------------------------------------------------------------
|       DEFPL macro
|
|       Used to define a specific plex.
|
|       Arguments:
|               PLTYP           name of the plex type
|               TYP             type of item stored in the plex
|               iMax            name to use for the iMax field
|               iMac            name to use for the iMac field
|               rg              name to use for the rg field
----------------------------------------------------------------------------*/
#define DEFPL(PLTYP,TYP,iMax,iMac,rg) \
        typedef struct PLTYP\
                { \
                WORD iMax : 15, \
                        fUseCount : 1; \
                SHORT iMac; \
                WORD cbItem; \
                WORD dAlloc:5, \
                        unused:11; \
                TYP rg[1]; \
                } \
            PLTYP;


/*----------------------------------------------------------------------------
|       DEFPL2 macro
|
|       Used to define a specific plex.
|
|       Arguments:
|               PLST            name of the plex struct
|               PLTYP           name of the plex type
|               TYP             type of item stored in the plex
|               iMax            name to use for the iMax field
|               iMac            name to use for the iMac field
|               rg              name to use for the rg field
----------------------------------------------------------------------------*/
#define DEFPL2(PLST,PLTYP,TYP,iMax,iMac,rg) \
        typedef struct PLST\
                { \
                WORD iMax : 15, \
                        fUseCount : 1; \
                SHORT iMac; \
                WORD cbItem; \
                WORD dAlloc:5, \
                        dgShift:3, \
                        unused:8; \
                TYP rg[1]; \
                } \
            PLTYP;


// a FORPLEX was expanded by hand in bar.c:FHptbFromBarId for speed --
// if you change this then you may need to change that
#define FORPLEX(hp, hpMac, hppl) \
                        for ((hpMac) = ((hp) = (VOID HUGE *)((PL HUGE *)(hppl))->rg) + \
                                                 ((PL HUGE *)(hppl))->iMac; \
                                 LOWORD(hp) < LOWORD(hpMac); (hp)++)

#define cbPL ((int)&((PL *)0)->rg)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

VOID *PplAlloc(unsigned, int, unsigned);
int IAddPl(VOID **, VOID *);
VOID FreePpl(VOID *);
BOOL RemovePl(VOID *, int);
int IAddPlSort(VOID **, VOID *, int (*pfnSgn)());
BOOL FLookupSortedPl(VOID *, VOID *, int *, int (*pfnSgn)());
int ILookupPl(VOID *, VOID *, int (*pfnSgn)());
VOID *PLookupPl(VOID *, VOID *, int (*pfnSgn)());
int CbPlAlloc(VOID *);
int IAddNewPlPos(VOID **, VOID *, int, int);

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif /* __plex__ */

