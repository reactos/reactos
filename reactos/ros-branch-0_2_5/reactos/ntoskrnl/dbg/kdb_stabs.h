#ifndef __KDB_STABS_H__
#define __KDB_STABS_H__

typedef struct _STAB_ENTRY {
  unsigned long n_strx;         /* index into string table of name */
  unsigned char n_type;         /* type of symbol */
  unsigned char n_other;        /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned long n_value;        /* value of symbol */
} STAB_ENTRY, *PSTAB_ENTRY;

/*
 * String - Function name with type information
 * Desc - Line number
 * Value - Absolute virtual address
 */
#define N_FUN 0x24

/*
 * Desc - Line number
 * Value - Relative virtual address
 */
#define N_SLINE 0x44

/*
 * String - First ending with a '/' is the compillation directory (CD)
 *          Not ending with a '/' is a source file relative to CD
 */
#define N_SO 0x64

/*
 * String - Variable name with type information
 * Value - Register
 */
#define N_RSYM 0x40

/*
 * String - Variable name with type information
 * Value - Offset of variable within local variables (from %ebp)
 */
#define N_LSYM 0x80

#define N_SOL 0x84


#define N_PSYM 0xa0

/*
 * Value - Start address of code block
 */
#define N_LBRAC 0xc0

/*
 * Value - End address of code block
 */
#define N_RBRAC 0xe0


/*
 * Functions
 */

PSTAB_ENTRY
KdbpStabFindEntry(IN PIMAGE_SYMBOL_INFO SymbolInfo,
                  IN CHAR Type,
                  IN PVOID RelativeAddress  OPTIONAL,
                  IN PSTAB_ENTRY StartEntry  OPTIONAL);

#endif /* __KDB_STABS_H__ */
