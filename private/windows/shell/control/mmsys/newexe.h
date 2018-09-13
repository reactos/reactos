/*static char *SCCSID = "@(#)newexe.h:2.9";*/
/*
 *  Title
 *
 *      newexe.h
 *      Pete Stewart
 *      (C) Copyright Microsoft Corp 1984
 *      17 August 1984
 *
 *  Description
 *
 *      Data structure definitions for the DOS 4.0/Windows 2.0
 *      executable file format.
 *
 *  Modification History
 *
 *      84/08/17        Pete Stewart    Initial version
 *      84/10/17        Pete Stewart    Changed some constants to match OMF
 *      84/10/23        Pete Stewart    Updates to match .EXE format revision
 *      84/11/20        Pete Stewart    Substantial .EXE format revision
 *      85/01/09        Pete Stewart    Added constants ENEWEXE and ENEWHDR
 *      85/01/10        Steve Wood      Added resource definitions
 *      85/03/04        Vic Heller      Reconciled Windows and DOS 4.0 versions
 *      85/03/07        Pete Stewart    Added movable entry count
 *      85/04/01        Pete Stewart    Segment alignment field, error bit
 */

#define EMAGIC          0x5A4D          /* Old magic number */
#define ENEWEXE         sizeof(struct exe_hdr)
                                        /* Value of E_LFARLC for new .EXEs */
#define ENEWHDR         0x003C          /* Offset in old hdr. of ptr. to new */
#define ERESWDS         0x0010          /* No. of reserved words in header */
#define ECP             0x0004          /* Offset in struct of E_CP */
#define ECBLP           0x0002          /* Offset in struct of E_CBLP */
#define EMINALLOC       0x000A          /* Offset in struct of E_MINALLOC */

struct exe_hdr                          /* DOS 1, 2, 3 .EXE header */
  {
    unsigned short      e_magic;        /* Magic number */
    unsigned short      e_cblp;         /* Bytes on last page of file */
    unsigned short      e_cp;           /* Pages in file */
    unsigned short      e_crlc;         /* Relocations */
    unsigned short      e_cparhdr;      /* Size of header in paragraphs */
    unsigned short      e_minalloc;     /* Minimum extra paragraphs needed */
    unsigned short      e_maxalloc;     /* Maximum extra paragraphs needed */
    unsigned short      e_ss;           /* Initial (relative) SS value */
    unsigned short      e_sp;           /* Initial SP value */
    unsigned short      e_csum;         /* Checksum */
    unsigned short      e_ip;           /* Initial IP value */
    unsigned short      e_cs;           /* Initial (relative) CS value */
    unsigned short      e_lfarlc;       /* File address of relocation table */
    unsigned short      e_ovno;         /* Overlay number */
    unsigned short      e_res[ERESWDS]; /* Reserved words */
    long                e_lfanew;       /* File address of new exe header */
  };

#define E_MAGIC(x)      (x).e_magic
#define E_CBLP(x)       (x).e_cblp
#define E_CP(x)         (x).e_cp
#define E_CRLC(x)       (x).e_crlc
#define E_CPARHDR(x)    (x).e_cparhdr
#define E_MINALLOC(x)   (x).e_minalloc
#define E_MAXALLOC(x)   (x).e_maxalloc
#define E_SS(x)         (x).e_ss
#define E_SP(x)         (x).e_sp
#define E_CSUM(x)       (x).e_csum
#define E_IP(x)         (x).e_ip
#define E_CS(x)         (x).e_cs
#define E_LFARLC(x)     (x).e_lfarlc
#define E_OVNO(x)       (x).e_ovno
#define E_RES(x)        (x).e_res
#define E_LFANEW(x)     (x).e_lfanew

#define NEMAGIC         0x454E          /* New magic number */
#define NERESBYTES      0

struct new_exe                          /* New .EXE header */
  {
    unsigned short int  ne_magic;       /* Magic number NE_MAGIC */
    char                ne_ver;         /* Version number */
    char                ne_rev;         /* Revision number */
    unsigned short int  ne_enttab;      /* Offset of Entry Table */
    unsigned short int  ne_cbenttab;    /* Number of bytes in Entry Table */
    long                ne_crc;         /* Checksum of whole file */
    unsigned short int  ne_flags;       /* Flag word */
    unsigned short int  ne_autodata;    /* Automatic data segment number */
    unsigned short int  ne_heap;        /* Initial heap allocation */
    unsigned short int  ne_stack;       /* Initial stack allocation */
    long                ne_csip;        /* Initial CS:IP setting */
    long                ne_sssp;        /* Initial SS:SP setting */
    unsigned short int  ne_cseg;        /* Count of file segments */
    unsigned short int  ne_cmod;        /* Entries in Module Reference Table */
    unsigned short int  ne_cbnrestab;   /* Size of non-resident name table */
    unsigned short int  ne_segtab;      /* Offset of Segment Table */
    unsigned short int  ne_rsrctab;     /* Offset of Resource Table */
    unsigned short int  ne_restab;      /* Offset of resident name table */
    unsigned short int  ne_modtab;      /* Offset of Module Reference Table */
    unsigned short int  ne_imptab;      /* Offset of Imported Names Table */
    long                ne_nrestab;     /* Offset of Non-resident Names Table */
    unsigned short int  ne_cmovent;     /* Count of movable entries */
    unsigned short int  ne_align;       /* Segment alignment shift count */
    unsigned short int  ne_cres;        /* Count of resource segments */

    unsigned short int  ne_psegcsum;    /* offset to segment chksums */
    unsigned short int  ne_pretthunks;  /* offset to return thunks */
    unsigned short int  ne_psegrefbytes;/* offset to segment ref. bytes */
    unsigned short int  ne_swaparea;    /* Minimum code swap area size */
    unsigned short int  ne_expver;      /* Expected Windows version number */
  };

#define NE_MAGIC(x)     (x).ne_magic
#define NE_VER(x)       (x).ne_ver
#define NE_REV(x)       (x).ne_rev
#define NE_ENTTAB(x)    (x).ne_enttab
#define NE_CBENTTAB(x)  (x).ne_cbenttab
#define NE_CRC(x)       (x).ne_crc
#define NE_FLAGS(x)     (x).ne_flags
#define NE_AUTODATA(x)  (x).ne_autodata
#define NE_HEAP(x)      (x).ne_heap
#define NE_STACK(x)     (x).ne_stack
#define NE_CSIP(x)      (x).ne_csip
#define NE_SSSP(x)      (x).ne_sssp
#define NE_CSEG(x)      (x).ne_cseg
#define NE_CMOD(x)      (x).ne_cmod
#define NE_CBNRESTAB(x) (x).ne_cbnrestab
#define NE_SEGTAB(x)    (x).ne_segtab
#define NE_RSRCTAB(x)   (x).ne_rsrctab
#define NE_RESTAB(x)    (x).ne_restab
#define NE_MODTAB(x)    (x).ne_modtab
#define NE_IMPTAB(x)    (x).ne_imptab
#define NE_NRESTAB(x)   (x).ne_nrestab
#define NE_CMOVENT(x)   (x).ne_cmovent
#define NE_ALIGN(x)     (x).ne_align
#define NE_RES(x)       (x).ne_res

#define NE_USAGE(x)     (WORD)*((WORD FAR *)(x)+1)
#define NE_PNEXTEXE(x)  (WORD)(x).ne_cbenttab
#define NE_PAUTODATA(x) (WORD)(x).ne_crc
#define NE_PFILEINFO(x) (WORD)((DWORD)(x).ne_crc >> 16)

#ifdef DOS5
#define NE_MTE(x)   (x).ne_psegcsum /* DOS 5 MTE handle for this module */
#endif


/*
 *  Format of NE_FLAGS(x):
 *
 *  p                                   Not-a-process
 *   c                                  Non-conforming
 *    e                                 Errors in image
 *     xxxxxxxxx                        Unused
 *              P                       Runs in protected mode
 *               r                      Runs in real mode
 *                i                     Instance data
 *                 s                    Solo data
 */
#define NENOTP          0x8000          /* Not a process */
#define NENONC          0x4000          /* Non-conforming program */
#define NEIERR          0x2000          /* Errors in image */
#define NEPROT          0x0008          /* Runs in protected mode */
#define NEREAL          0x0004          /* Runs in real mode */
#define NEINST          0x0002          /* Instance data */
#define NESOLO          0x0001          /* Solo data */

struct new_seg                          /* New .EXE segment table entry */
  {
    unsigned short      ns_sector;      /* File sector of start of segment */
    unsigned short      ns_cbseg;       /* Number of bytes in file */
    unsigned short      ns_flags;       /* Attribute flags */
    unsigned short      ns_minalloc;    /* Minimum allocation in bytes */
  };

struct new_seg1                         /* New .EXE segment table entry */
  {
    unsigned short      ns_sector;      /* File sector of start of segment */
    unsigned short      ns_cbseg;       /* Number of bytes in file */
    unsigned short      ns_flags;       /* Attribute flags */
    unsigned short      ns_minalloc;    /* Minimum allocation in bytes */
    unsigned short      ns_handle;      /* Handle of segment */
  };

#define NS_SECTOR(x)    (x).ns_sector
#define NS_CBSEG(x)     (x).ns_cbseg
#define NS_FLAGS(x)     (x).ns_flags
#define NS_MINALLOC(x)  (x).ns_minalloc

/*
 *  Format of NS_FLAGS(x):
 *
 *  xxxx                                Unused
 *      DD                              286 DPL bits
 *        d                             Segment has debug info
 *         r                            Segment has relocations
 *          e                           Execute/read only
 *           p                          Preload segment
 *            P                         Pure segment
 *             m                        Movable segment
 *              i                       Iterated segment
 *               ttt                    Segment type
 */
#define NSTYPE          0x0007          /* Segment type mask */
#define NSCODE          0x0000          /* Code segment */
#define NSDATA          0x0001          /* Data segment */
#define NSITER          0x0008          /* Iterated segment flag */
#define NSMOVE          0x0010          /* Movable segment flag */
#define NSPURE          0x0020          /* Pure segment flag */
#define NSPRELOAD       0x0040          /* Preload segment flag */
#define NSEXRD          0x0080          /* Execute-only (code segment), or
                                        *  read-only (data segment)
                                        */
#define NSRELOC         0x0100          /* Segment has relocations */
#define NSDEBUG         0x0200          /* Segment has debug info */
#define NSDPL           0x0C00          /* 286 DPL bits */
#define NSDISCARD       0x1000          /* Discard bit for segment */

#define NSALIGN 9       /* Segment data aligned on 512 byte boundaries */

struct new_segdata                      /* Segment data */
  {
    union
      {
        struct
          {
            unsigned short      ns_niter;       /* number of iterations */
            unsigned short      ns_nbytes;      /* number of bytes */
            char                ns_iterdata;    /* iterated data bytes */
          } ns_iter;
        struct
          {
            char                ns_data;        /* data bytes */
          } ns_noniter;
      } ns_union;
  };

struct new_rlcinfo                      /* Relocation info */
  {
    unsigned short      nr_nreloc;      /* number of relocation items that */
  };                                    /* follow */

struct new_rlc                          /* Relocation item */
  {
    char                nr_stype;       /* Source type */
    char                nr_flags;       /* Flag byte */
    unsigned short      nr_soff;        /* Source offset */
    union
      {
        struct
          {
            char        nr_segno;       /* Target segment number */
            char        nr_res;         /* Reserved */
            unsigned short nr_entry;    /* Target Entry Table offset */
          }             nr_intref;      /* Internal reference */
        struct
          {
            unsigned short nr_mod;      /* Index into Module Reference Table */
            unsigned short nr_proc;     /* Procedure ordinal or name offset */
          }             nr_import;      /* Import */
      }                 nr_union;       /* Union */
  };

#define NR_STYPE(x)     (x).nr_stype
#define NR_FLAGS(x)     (x).nr_flags
#define NR_SOFF(x)      (x).nr_soff
#define NR_SEGNO(x)     (x).nr_union.nr_intref.nr_segno
#define NR_RES(x)       (x).nr_union.nr_intref.nr_res
#define NR_ENTRY(x)     (x).nr_union.nr_intref.nr_entry
#define NR_MOD(x)       (x).nr_union.nr_import.nr_mod
#define NR_PROC(x)      (x).nr_union.nr_import.nr_proc

/*
 *  Format of NR_STYPE(x):
 *
 *  xxxxx                               Unused
 *       sss                            Source type
 */
#define NRSTYP          0x07            /* Source type mask */
#define NRSSEG          0x02            /* 16-bit segment */
#define NRSPTR          0x03            /* 32-bit pointer */
#define NRSOFF          0x05            /* 16-bit offset */

/*
 *  Format of NR_FLAGS(x):
 *
 *  xxxxx                               Unused
 *       a                              Additive fixup
 *        rr                            Reference type
 */
#define NRADD           0x04            /* Additive fixup */
#define NRRTYP          0x03            /* Reference type mask */
#define NRRINT          0x00            /* Internal reference */
#define NRRORD          0x01            /* Import by ordinal */
#define NRRNAM          0x02            /* Import by name */
#define OSFIXUP 	0x03		/* Floating point fixup */


/* Resource type or name string */
struct rsrc_string
    {
    char rs_len;            /* number of bytes in string */
    char rs_string[ 1 ];    /* text of string */
    };

#define RS_LEN( x )    (x).rs_len
#define RS_STRING( x ) (x).rs_string

/* Resource type information block */
struct rsrc_typeinfo
    {
    unsigned short rt_id;
    unsigned short rt_nres;
    long rt_proc;
    };

#define RT_ID( x )   (x).rt_id
#define RT_NRES( x ) (x).rt_nres
#define RT_PROC( x ) (x).rt_proc

/* Resource name information block */
struct rsrc_nameinfo
    {
    /* The following two fields must be shifted left by the value of  */
    /* the rs_align field to compute their actual value.  This allows */
    /* resources to be larger than 64k, but they do not need to be    */
    /* aligned on 512 byte boundaries, the way segments are           */
    unsigned short rn_offset;   /* file offset to resource data */
    unsigned short rn_length;   /* length of resource data */
    unsigned short rn_flags;    /* resource flags */
    unsigned short rn_id;       /* resource name id */
    unsigned short rn_handle;   /* If loaded, then global handle */
    unsigned short rn_usage;    /* Initially zero.  Number of times */
                                /* the handle for this resource has */
                                /* been given out */
    };

#define RN_OFFSET( x ) (x).rn_offset
#define RN_LENGTH( x ) (x).rn_length
#define RN_FLAGS( x )  (x).rn_flags
#define RN_ID( x )     (x).rn_id
#define RN_HANDLE( x ) (x).rn_handle
#define RN_USAGE( x )  (x).rn_usage

#define RSORDID     0x8000      /* if high bit of ID set then integer id */
                                /* otherwise ID is offset of string from
                                   the beginning of the resource table */

                                /* Ideally these are the same as the */
                                /* corresponding segment flags */
#define RNMOVE      0x0010      /* Moveable resource */
#define RNPURE      0x0020      /* Pure (read-only) resource */
#define RNPRELOAD   0x0040      /* Preloaded resource */
#define RNDISCARD   0x1000      /* Discard bit for resource */

#define RNLOADED    0x0004      /* True if handler proc return handle */

/* Resource table */
struct new_rsrc
    {
    unsigned short rs_align;    /* alignment shift count for resources */
    struct rsrc_typeinfo rs_typeinfo;
    };

#define RS_ALIGN( x ) (x).rs_align

#define FOPEN(sz)                _lopen(sz,OF_READ)
#define FCREATE(sz)              _lcreat(sz,0)
#define FCLOSE(fh)               _lclose(fh)
#define FREAD(fh,buf,len)        _lread(fh,buf,len)
#define FWRITE(fh,buf,len)       _lwrite(fh,buf,len)
#define FSEEK(fh,off,i)          _llseek(fh,(DWORD)off,i)
