/*  re_mi.c - machine independent regular expression compiler
 *  cl /c /Zep /AM /NT RE /Gs /G2 /Oa /D LINT_ARGS /Fc re_mi.c
 *
 *  Modifications:
 *      09-Mar-1988 mz  Add check in fREMtch for pat == NULL
 *      15-Sep-1988 bw  Change fREMatch to REMatch.  New parameters and
 *                      return type.
 *      23-Nov-1989 bp  Use relative adresses: OFST and PNTR macros
 *      05-Feb-1991 mz  Merge in KANJI changes
 *
 */

#include "precomp.h"
#pragma hdrstop


typedef unsigned short int CHAR_DBCS;

/*  The following are dependent on the low-level representation of the compiled
 *  machine.  The cases that have been implemented are:
 *
 *  Simple interpreted machine
 */

/* pseudo-instruction definitions */

#define I_CALL      0
#define I_RETURN    1
#define I_LETTER    2
#define I_ANY       3
#define I_EOL       4
#define I_BOL       5
#define I_CCL       6
#define I_NCCL      7
#define I_MATCH     8
#define I_JMP       9
#define I_SPTOM     10
#define I_PTOM      11
#define I_MTOP      12
#define I_MTOSP     13
#define I_FAIL      14
#define I_PUSHP     15
#define I_PUSHM     16
#define I_POPP      17
#define I_POPM      18
#define I_PNEQM     19
#define I_ITOM      20
#define I_PREV      21

/* instruction templates and lengths */

#define LLETTER     3
#define LANY        1
#define LBOL        1
#define LEOL        1

/* address part of instruction */

#define ADDR(ip)    (*(UNALIGNED RE_OPCODE **)(ip+sizeof(RE_OPCODE)))

/* conversion macros for adresses */

#define OFST(p)     ((RE_OPCODE *) (((char *) p) - ((char *) REPat)))
#define PNTR(p)     ((RE_OPCODE *) (((char *) REPat) + ((ULONG_PTR) p)))

#define IMM(ip)     (*(UNALIGNED RE_OPCODE **)(ip+sizeof(RE_OPCODE)+sizeof(RE_OPCODE *)))

#define LRANGE      (sizeof(RE_OPCODE)+sizeof(RE_OPCODE)+sizeof(RE_OPCODE)+sizeof(RE_OPCODE))
#define LCCL        (sizeof(RE_OPCODE))
#define LNCCL       (sizeof(RE_OPCODE))

#define LOFFSET     sizeof(RE_OPCODE *)
#define LCALL       (sizeof(RE_OPCODE)+LOFFSET)
#define LJMP        (sizeof(RE_OPCODE)+LOFFSET)
#define LSPTOM      (sizeof(RE_OPCODE)+LOFFSET)
#define LPTOM       (sizeof(RE_OPCODE)+LOFFSET)
#define LMTOP       (sizeof(RE_OPCODE)+LOFFSET)
#define LMTOSP      (sizeof(RE_OPCODE)+LOFFSET)
#define LRETURN     sizeof(RE_OPCODE)
#define LMATCH      sizeof(RE_OPCODE)
#define LFAIL       sizeof(RE_OPCODE)
#define LPUSHM      (sizeof(RE_OPCODE)+LOFFSET)
#define LPOPM       (sizeof(RE_OPCODE)+LOFFSET)
#define LPUSHP      sizeof(RE_OPCODE)
#define LPOPP       sizeof(RE_OPCODE)
#define LPNEQM      (sizeof(RE_OPCODE)+LOFFSET)
#define LITOM       (sizeof(RE_OPCODE)+LOFFSET+LOFFSET)
#define LPREV       (sizeof(RE_OPCODE)+sizeof(RE_OPCODE))

/* action templates */

typedef struct {
         RE_OPCODE      i1[LCALL];              /*      CALL    pattern               */
         RE_OPCODE      i2[LFAIL];              /*      FAIL                          */
         } T_PROLOG;                    /* pattern:                           */

typedef struct {
         RE_OPCODE      i1[LPTOM];              /*      PTOM    ArgBeg[cArg]          */
         RE_OPCODE      i2[LCALL];              /*      CALL    x                     */
         RE_OPCODE      i3[LITOM];              /*      ITOM    ArgBeg[cArg],-1       */
         RE_OPCODE      i4[LRETURN];            /*      RETURN                        */
         } T_LEFTARG;                   /* x:                                 */

typedef struct {
         RE_OPCODE      i1[LPTOM];              /*      PTOM    ArgEnd[cArg]          */
         } T_RIGHTARG;

typedef struct {
         RE_OPCODE      i1[LPUSHM];             /*      PUSHM   tmp                   */
         RE_OPCODE      i2[LCALL];              /*      CALL    l1                    */
         RE_OPCODE      i3[LPOPM];              /*      POPM    tmp                   */
         RE_OPCODE      i4[LRETURN];            /*      RETURN                        */
         RE_OPCODE      tmp[LOFFSET];           /* tmp  DW                            */
         RE_OPCODE      i6[LPUSHP];             /* l1:  PUSHP                         */
         RE_OPCODE      i7[LCALL];              /*      CALL    y                     */
         RE_OPCODE      i8[LPOPP];              /*      POPP                          */
         RE_OPCODE      i9[LPTOM];              /*      PTOM    tmp                   */
         } T_SMSTAR;                    /* x:   ...                           */

typedef struct {
         RE_OPCODE      i1[LPNEQM];             /*      PNEQM   tmp                   */
         RE_OPCODE      i2[LJMP];               /*      JMP     l1                    */
         } T_SMSTAR1;                   /* y:   ...                           */

typedef struct {
         RE_OPCODE      i1[LPUSHM];             /* l1:  PUSHM   tmp                   */
         RE_OPCODE      i2[LPTOM];              /*      PTOM    tmp                   */
         RE_OPCODE      i3[LPUSHP];             /*      PUSHP                         */
         RE_OPCODE      i4[LCALL];              /*      CALL    x                     */
         RE_OPCODE      i5[LPOPP];              /*      POPP                          */
         RE_OPCODE      i6[LPOPM];              /*      POPM    tmp                   */
         RE_OPCODE      i7[LJMP];               /*      JMP     y                     */
         RE_OPCODE      tmp[LOFFSET];           /* tmp  DW                            */
         } T_STAR;                              /* x:   ...                           */

typedef struct {
         RE_OPCODE      i1[LPNEQM];             /*      PNEQM   tmp                   */
         RE_OPCODE      i2[LPTOM];              /*      PTOM    tmp                   */
         RE_OPCODE      i3[LJMP];               /*      JMP     l1                    */
         } T_STAR1;                             /* y:   ...                           */

typedef struct {
         RE_OPCODE      i1[LANY];               /*      ANY                           */
         } T_ANY;

typedef struct {
         RE_OPCODE      i1[LBOL];               /*      BOL                           */
         } T_BOL;

typedef struct {
         RE_OPCODE      i1[LEOL];               /*      EOL                           */
         } T_EOL;

typedef struct {
         RE_OPCODE      i1[LSPTOM];             /*      SPTOM   tmp                   */
         RE_OPCODE      i2[LPTOM];              /*      PTOM    tmp1                  */
         RE_OPCODE      i3[LCALL];              /*      CALL    x                     */
         RE_OPCODE      i4[LMTOP];              /*      MTOP    tmp1                  */
         RE_OPCODE      i5[LJMP];               /*      JMP     y                     */
         RE_OPCODE      tmp[LOFFSET];           /* tmp  DW                            */
         RE_OPCODE      tmp1[LOFFSET];          /* tmp1 DW                            */
         } T_NOTSIGN;                   /* x:   ...                           */

typedef struct {
         RE_OPCODE      i1[LMTOSP];             /*      MTOSP   tmp                   */
         RE_OPCODE      i2[LMTOP];              /*      MTOP    tmp1                  */
         RE_OPCODE      i3[LRETURN];            /*      RETURN                        */
         } T_NOTSIGN1;                  /* y:   ...                           */

typedef struct {
         RE_OPCODE      i1[LLETTER];            /*      LETTER  c                     */
         } T_LETTER;

typedef struct {
         RE_OPCODE      i1[LPUSHP];             /* ln:  PUSHP                         */
         RE_OPCODE      i2[LCALL];              /*      CALL    cn                    */
         RE_OPCODE      i3[LPOPP];              /*      POPP                          */
         RE_OPCODE      i4[LJMP];               /*      JMP     ln+1                  */
         } T_LEFTOR;                    /* cn:  ...                           */

typedef struct {
         RE_OPCODE      i1[LJMP];               /*      JMP     y                     */
         } T_ORSIGN;

typedef struct {
         RE_OPCODE      i1[LRETURN];            /* cn+1:RETURN                        */
         } T_RIGHTOR;                   /* y:   ...                           */

typedef struct {
         RE_OPCODE      i1[LCCL];               /*      CCL <bits>                    */
         } T_CCL;

typedef struct {
         RE_OPCODE      i1[LMATCH];             /*      MATCH                         */
         } T_EPILOG;

typedef struct {
         RE_OPCODE      i1[LPREV];              /*      PREV    n                     */
         } T_PREV;

typedef struct {
         RE_OPCODE      i1[LRANGE];             /*      x1:x2 - y1:y2                 */
         } T_RANGE;

typedef union {
         T_PROLOG       U_PROLOG;
         T_LEFTARG      U_LEFTARG;
         T_RIGHTARG     U_RIGHTARG;
         T_SMSTAR       U_SMSTAR;
         T_SMSTAR1      U_SMSTAR1;
         T_STAR U_STAR;
         T_STAR1        U_STAR1;
         T_ANY  U_ANY;
         T_BOL  U_BOL;
         T_EOL  U_EOL;
         T_NOTSIGN      U_NOTSIGN;
         T_NOTSIGN1     U_NOTSIGN1;
         T_LETTER       U_LETTER;
         T_LEFTOR       U_LEFTOR;
         T_ORSIGN       U_ORSIGN;
         T_RIGHTOR      U_RIGHTOR;
         T_CCL  U_CCL;
         T_EPILOG       U_EPILOG;
         T_PREV U_PREV;
         T_RANGE        U_RANGE;
         } _tagTemplate ;

/* size of each compiled action */

int cbIns[] =  {
/* PROLOG      0    */  sizeof (T_PROLOG      ),
/* LEFTARG     1    */  sizeof (T_LEFTARG     ),
/* RIGHTARG    2    */  sizeof (T_RIGHTARG    ),
/* SMSTAR      3    */  sizeof (T_SMSTAR      ),
/* SMSTAR1     4    */  sizeof (T_SMSTAR1     ),
/* STAR        5    */  sizeof (T_STAR        ),
/* STAR1       6    */  sizeof (T_STAR1       ),
/* ANY         7    */  sizeof (T_ANY         ),
/* BOL         8    */  sizeof (T_BOL         ),
/* EOL         9    */  sizeof (T_EOL         ),
/* NOTSIGN     10   */  sizeof (T_NOTSIGN     ),
/* NOTSIGN1    11   */  sizeof (T_NOTSIGN1    ),
/* LETTER      12   */  sizeof (T_LETTER      ),
/* LEFTOR      13   */  sizeof (T_LEFTOR      ),
/* ORSIGN      14   */  sizeof (T_ORSIGN      ),
/* RIGHTOR     15   */  sizeof (T_RIGHTOR     ),
/* CCLBEG      16   */  sizeof (T_CCL         ),
/* CCLNOT      17   */  sizeof (T_CCL         ),
/* RANGE       18   */  0,
/* EPILOG      19   */  sizeof (T_EPILOG      ),
/* PREV        20   */  sizeof (T_PREV        ),
/* RANGEDBCS1  21   */  sizeof (T_RANGE       ),
/* RANGEDBCS2  22   */  0,
                        0
                        };

#if DEBUG
#define DEBOUT(x)   printf x;
#else
#define DEBOUT(x)
#endif



/* MovePBackwards - Move P backwards
 *
 *
 */
unsigned char *MovePBackwards (unsigned char *bos, unsigned char *P)
{
        unsigned char *P1;

        if (P - 1 <= bos)
            return bos;

        P1 = P - 2;
        while (TRUE) {
            if ( P1 == bos || !IsDBCSLeadByte(*P1) )
                 break;
            P1--;
        }

        if (IsDBCSLeadByte(*P1))
            P1--;

        return P - 1 - ((P - P1) % 2);
}



/*  REMatch - enumerate all matches of a pattern onto a string
 *
 *      pat     compiled pattern (gotten from RECompile)
 *      bos     pointer to beginning of string to scan
 *      str     pointer to into bos of place to begin scan
 *      fFor    direction to move on unsuccessful compares (for <msearch> in Z)
 *      Stack   simulated stack                   
 *
 *  REMatch returns 0 if a match was found.  Otherwise it returns a non-zero
 *  error code.
 *
 *  REMatch interprets the compiled patching machine in the pattern.
 */
int REMatch (struct patType *pat, char *bos, char *str, RE_OPCODE *Stack[],
    unsigned  MaxREStack, flagType fFor)
{
         RE_OPCODE **SP;                        /* top of stack                      */
         register RE_OPCODE *IP;                /* current instruction to execute    */
         register unsigned char *P;             /* pointer to next char to match     */
         RE_OPCODE        C;
         unsigned short U;
         int fMatched;
         int i, n;
         RE_OPCODE **StackEnd = & Stack[MaxREStack-sizeof(Stack[0])];
         int (_cdecl * pfncomp) (const char *, const char *, size_t);

         if ((REPat = pat) == NULL)
        return REM_INVALID;

         pfncomp = REPat->fCase ? strncmp : _strnicmp;

         /* initialize the machine */
         Fill ((char far *) REPat->pArgBeg, -1, sizeof (REPat->pArgBeg));
         REPat->pArgBeg[0] = str;
         P = (PUCHAR) str;

         /* begin this instance of the machine */
         SP = &Stack[-1];
         IP = REPat->code;

         while (TRUE) {
        DEBOUT (("%04x/%04x/%04x ", IP, SP-&Stack[0], P));
        /* execute instruction */
        switch (*IP) {
        /* call a subroutine */
        case I_CALL:
                 if (SP >= StackEnd)
                return REM_STKOVR;
                 *++SP = IP + LCALL;
                 IP = PNTR (ADDR (IP));
                 DEBOUT (("CALL %04x\n", IP));
                 break;

        /* return from a subroutine */
        case I_RETURN:
                 DEBOUT (("RETURN\n"));
                 IP = *SP--;
                 break;

        /* match a character, fail if no match */
        case I_LETTER:
                if (IsDBCSLeadByte(*P)) {
                    if (*(CHAR_DBCS *)P == *(CHAR_DBCS *)(IP+1))
                         IP += LLETTER;
                    else
                         IP = *SP--;
                    P += 2;
                }
                else {
                    C = REPat->fCase ? *P++ : XLTab[*P++];
                    if (C == IP[1])
                         IP += LLETTER;
                    else
                         IP = *SP--;
                }
                 break;

        /* match any character, fail if no match */
        case I_ANY:
                DEBOUT (("ANY\n"));
                if (*P != '\0') {
                    if (IsDBCSLeadByte(*P))
                        P++;
                    IP += LANY;
                }
                else
                    IP = *SP--;
                P++;
                 break;

        /* match end of line, fail if no match */
        case I_EOL:
                 DEBOUT (("EOL\n"));
                 if (*P == '\0')
                IP += LEOL;
                 else
                IP = *SP--;
                 break;

        /* match beginning of line, fail if no match */
        case I_BOL:
                 DEBOUT (("BOL\n"));
                 if (P == (PUCHAR) bos)
                IP += LBOL;
                 else
                IP = *SP--;
                 break;

        /* handle character class, fail if no match */
        case I_CCL:
                IP += LCCL;
                fMatched = FALSE;
                U = REPat->fCase ? *P++ : (unsigned char)XLTab[*P++];
                if (IsDBCSLeadByte((BYTE)U))
                    U = (U << 8) + *P++;
                if (C == '\0') {
                    IP = *SP--;
                    break;
                }
#define USIP    ((unsigned short *)IP)
#define CLOW    (USIP[0])
#define CHIGH   (USIP[1])
                while (CLOW != 0) {
                    fMatched |= (CLOW <= U) && (U <= CHIGH);
                    IP += LRANGE;
                }
                IP += LRANGE;
#undef USIP
#undef CLOW
#undef CHIGH
                if (!fMatched)
                    IP = *SP--;

                 break;

        /* handle not character class, fail if match */
        case I_NCCL:
                 IP += LCCL;
                 fMatched = FALSE;
                 U = REPat->fCase ? *P++ : (unsigned char)XLTab[*P++];
                 if (IsDBCSLeadByte((BYTE)U))
                U = (U << 8) + *P++;
                 if (C == '\0') {
                IP = *SP--;
                break;
                }
#define USIP    ((unsigned short *)IP)
#define CLOW    (USIP[0])
#define CHIGH   (USIP[1])
                 while (CLOW != 0) {
                fMatched |= (CLOW <= U) && (U <= CHIGH);
                IP += LRANGE;
                }
                 IP += LRANGE;
#undef USIP
#undef CLOW
#undef CHIGH
                 if (fMatched)
                IP = *SP--;

                 break;

        /* signal a match */
        case I_MATCH:
                 DEBOUT (("MATCH\n"));
                 REPat->pArgEnd[0] = (PSTR) P;
                 return REM_MATCH;

        /* jump to an instruction */
        case I_JMP:
                 IP = PNTR (ADDR (IP));
                 DEBOUT (("JMP %04x\n", IP));
                 break;

        /* save the character pointer in a memory location */
        case I_PTOM:
                 DEBOUT (("PTOM %04x\n", PNTR (ADDR(IP))));
                 * ((unsigned char **) PNTR (ADDR (IP))) = P;
                 IP += LPTOM;
                 break;

        /* restore the character pointer from a memory location */
        case I_MTOP:
                 DEBOUT (("MTOP %04x\n", PNTR (ADDR(IP))));
                 P = * ((unsigned char **) PNTR (ADDR (IP)));
                 IP += LMTOP;
                 break;

        /* save the stack pointer in a memory location */
        case I_SPTOM:
                 DEBOUT (("SPTOM %04x\n", PNTR (ADDR(IP))));
                 * ((RE_OPCODE ***) PNTR (ADDR (IP))) = SP;
                 IP += LSPTOM;
                 break;

        /* restore the stack pointer from a memory location */
        case I_MTOSP:
                 DEBOUT (("MTOSP %04x\n", PNTR (ADDR (IP))));
                 SP = * ((RE_OPCODE ***) PNTR (ADDR (IP)));
                 IP += LMTOSP;
                 break;

        /* push the char pointer */
        case I_PUSHP:
                 DEBOUT (("PUSHP\n"));
                 if (SP >= StackEnd)
                return REM_STKOVR;
                 *++SP = (RE_OPCODE *) P;
                 IP++;
                 break;

        /* pop the char pointer */
        case I_POPP:
                 DEBOUT (("POPP\n"));
                 P = (unsigned char *) (*SP--);
                 IP ++;
                 break;

        /* push memory */
        case I_PUSHM:
                 DEBOUT (("PUSHM %04x\n", PNTR (ADDR (IP))));
                 if (SP >= StackEnd)
                return REM_STKOVR;
                 *++SP = * ((RE_OPCODE **) PNTR (ADDR (IP)));
                 IP += LPUSHM;
                 break;

        /* pop memory */
        case I_POPM:
                 DEBOUT (("POPM %04x\n", PNTR (ADDR (IP))));
                 * ((RE_OPCODE **) PNTR (ADDR (IP))) = *SP--;
                 IP += LPOPM;
                 break;

        /* make sure that the char pointer P is != memory, fail if necessary */
        case I_PNEQM:
                 DEBOUT (("PNEQM %04x\n", PNTR (ADDR (IP))));
                 if (P != * ((unsigned char **) PNTR (ADDR (IP))))
                IP += LPNEQM;
                 else
                IP = *SP--;
                 break;

        /* move an immediate value to memory */
        case I_ITOM:
                 DEBOUT (("ITOM %04x,%04x\n", PNTR (ADDR (IP)), IMM(IP)));
                 * ((RE_OPCODE **) PNTR (ADDR (IP))) = IMM (IP);
                 IP += LITOM;
                 break;

        /* indicate a fail on the total match */
        case I_FAIL:
                 DEBOUT (("FAIL\n"));
                 P = (PUCHAR) REPat->pArgBeg[0];
                 if (fFor) {
                if (IsDBCSLeadByte(*P))
                         P++;
                if (*P++ == '\0')
                         return REM_NOMATCH;
                else
                         ;
                 }
                 else
                 if (P == (PUCHAR) bos)
                return REM_NOMATCH;
                 else
                P = MovePBackwards ((PUCHAR) bos, P);


                 REPat->pArgBeg[0] = (PSTR) P;
                 SP = &Stack[-1];
                 IP = REPat->code;
                 break;

        /* perform a match with a previously matched item */
        case I_PREV:
                 i = IP[1];
                 n = (int) (REPat->pArgEnd[i] - REPat->pArgBeg[i]);
                 DEBOUT (("PREV %04x\n", i));
                 if (REPat->pArgBeg[i] == (char *) -1)
                IP = *SP--;
                 else
                 if ((*pfncomp) (REPat->pArgBeg[i], (PCSTR) P, n))
                IP = *SP--;
                 else {
                IP += LPREV;
                P += n;
                }
                 break;
        default:
                 return REM_UNDEF;

                 }
        }
}

void pascal INTERNAL REStackOverflow ()
{
        InternalErrorBox(SYS_RegExpr_StackOverflow);
}

/*  CompileAction - drop in the compilation template at a particular node
 *  in the tree.  Continuation appropriate to a node occurs by relying on
 *  passed input and past input (yuk, yuk).
 *
 *  type        type of action being performed
 *  u           previous return value.  Typically points to a previous
 *              template that needs to be linked together.
 *  x           low byte of a range
 *  y           high range of a range.
 *
 *  Returns     variable depending on action required.
 *
 */
UINT_PTR INTERNAL CompileAction (OPTYPE type, register UINT_PTR u,
    unsigned char x, unsigned char y)
{
         register _tagTemplate *t = (_tagTemplate *) REip;
         UINT_PTR u1, u2, u3;

         DEBOUT (("%04x CompileAction %04x\n", REip, type));

         REip += cbIns[type];

         switch (type) {

         case PROLOG:
#define ip  (&(t->U_PROLOG))
        ip->i1[0] = I_CALL;     ADDR(ip->i1) = OFST (REip);
        ip->i2[0] = I_FAIL;
        return (UINT_PTR) NULL;
#undef  ip
        break;

         case LEFTARG:
#define ip  (&(t->U_LEFTARG))
        ip->i1[0] = I_PTOM;
        ADDR(ip->i1) = OFST ((RE_OPCODE *) &(REPat->pArgBeg[REArg]));
        ip->i2[0] = I_CALL;     ADDR(ip->i2) = OFST (REip);
        ip->i3[0] = I_ITOM;
        ADDR(ip->i3) = OFST ((RE_OPCODE *) &(REPat->pArgBeg[REArg]));
        IMM(ip->i3) = (RE_OPCODE *) -1;
        ip->i4[0] = I_RETURN;
        return (UINT_PTR) REArg++;
#undef  ip
        break;

         case RIGHTARG:
#define ip  (&(t->U_RIGHTARG))
        ip->i1[0] = I_PTOM;
        ADDR(ip->i1) = OFST ((RE_OPCODE *) &(REPat->pArgEnd[u]));
        return (UINT_PTR) NULL;
#undef  ip
        break;

         case SMSTAR:
#define ip  (&(t->U_SMSTAR))
        return (UINT_PTR) ip;
#undef  ip
        break;

         case SMSTAR1:
#define ip  ((T_SMSTAR *)u)
#define ip2 (&(t->U_SMSTAR1))
        ip->i1[0] = I_PUSHM;    ADDR(ip->i1) = OFST (ip->tmp);
        ip->i2[0] = I_CALL;     ADDR(ip->i2) = OFST (ip->i6);
        ip->i3[0] = I_POPM;     ADDR(ip->i3) = OFST (ip->tmp);
        ip->i4[0] = I_RETURN;
        /* DW */
        ip->i6[0] = I_PUSHP;
        ip->i7[0] = I_CALL;     ADDR(ip->i7) = OFST (REip);
        ip->i8[0] = I_POPP;
        ip->i9[0] = I_PTOM;     ADDR(ip->i9) = OFST (ip->tmp);

        ip2->i1[0] = I_PNEQM;   ADDR(ip2->i1) = OFST (ip->tmp);
        ip2->i2[0] = I_JMP;     ADDR(ip2->i2) = OFST (ip->i6);
        return (UINT_PTR) NULL;
#undef  ip
#undef  ip2
        break;

         case STAR:
#define ip  (&(t->U_STAR))
        return (UINT_PTR) ip;
#undef  ip
        break;

         case STAR1:
#define ip  ((T_STAR *)u)
#define ip2 (&(t->U_STAR1))
        ip->i1[0] = I_PUSHM;    ADDR(ip->i1) = OFST (ip->tmp);
        ip->i2[0] = I_PTOM;     ADDR(ip->i2) = OFST (ip->tmp);
        ip->i3[0] = I_PUSHP;
        ip->i4[0] = I_CALL;     ADDR(ip->i4) = OFST (((RE_OPCODE *)ip) + sizeof (*ip));
        ip->i5[0] = I_POPP;
        ip->i6[0] = I_POPM;     ADDR(ip->i6) = OFST (ip->tmp);
        ip->i7[0] = I_JMP;      ADDR(ip->i7) = OFST (REip);

        ip2->i1[0] = I_PNEQM;   ADDR(ip2->i1) = OFST (ip->tmp);
        ip2->i2[0] = I_PTOM;    ADDR(ip2->i2) = OFST (ip->tmp);
        ip2->i3[0] = I_JMP;     ADDR(ip2->i3) = OFST (ip->i1);
        return (UINT_PTR) NULL;
#undef  ip
#undef  ip2
        break;

         case ANY:
#define ip  (&(t->U_ANY))
        ip->i1[0] = I_ANY;
        return (UINT_PTR) NULL;
#undef  ip
        break;

         case BOL:
#define ip  (&(t->U_BOL))
        ip->i1[0] = I_BOL;
        return (UINT_PTR) NULL;
#undef  ip
        break;

         case EOL:
#define ip  (&(t->U_EOL))
        ip->i1[0] = I_EOL;
        return (UINT_PTR) NULL;
#undef  ip
        break;

         case NOTSIGN:
#define ip  (&(t->U_NOTSIGN))
        return (UINT_PTR) ip;
#undef  ip
        break;

         case NOTSIGN1:
#define ip  ((T_NOTSIGN *)u)
#define ip2 (&(t->U_NOTSIGN1))
        ip->i1[0] = I_SPTOM;    ADDR(ip->i1) = OFST (ip->tmp);
        ip->i2[0] = I_PTOM;     ADDR(ip->i2) = OFST (ip->tmp1);
        ip->i3[0] = I_CALL;     ADDR(ip->i3) = OFST (((RE_OPCODE *)ip) + sizeof (*ip));
        ip->i4[0] = I_MTOP;     ADDR(ip->i4) = OFST (ip->tmp1);
        ip->i5[0] = I_JMP;      ADDR(ip->i5) = OFST (REip);

        ip2->i1[0] = I_MTOSP;   ADDR(ip2->i1) = OFST (ip->tmp);
        ip2->i2[0] = I_MTOP;    ADDR(ip2->i2) = OFST (ip->tmp1);
        ip2->i3[0] = I_RETURN;
        return (UINT_PTR) NULL;
#undef  ip
#undef  ip2
        break;

         case LETTER:
#define ip  (&(t->U_LETTER))
        if (!REPat->fCase)
                 x = XLTab[x];
        ip->i1[0] = I_LETTER;   ip->i1[1] = (RE_OPCODE) x;
        ip->i1[2] = (RE_OPCODE) y;
        return (UINT_PTR) NULL;
#undef  ip
        break;

         case LEFTOR:
#define ip  (&(t->U_LEFTOR))
        * (UINT_PTR *) ip = u;
        return (UINT_PTR) ip;
#undef  ip
        break;

         case ORSIGN:
#define ip  (&(t->U_ORSIGN))
        * (UINT_PTR *) ip = u;
        return (UINT_PTR) ip;
#undef  ip
        break;

         case RIGHTOR:
        u1 = u;
        u2 = (UINT_PTR) t;
        u = * (UINT_PTR *) u1;
        while (u1 != (UINT_PTR) NULL) {
                 u3 = * (UINT_PTR *) u;
                 /*     u   points to leftor
                  *     u1  points to orsign
                  *     u2  points to next leftor
                  *     u3  points to previous orsign
                  */
#define ip  (&(((_tagTemplate *)u)->U_LEFTOR))
                 ip->i1[0] = I_PUSHP;
                 ip->i2[0] = I_CALL; ADDR (ip->i2) = OFST (((RE_OPCODE *)ip) + sizeof (*ip));
                 ip->i3[0] = I_POPP;
                 ip->i4[0] = I_JMP;     ADDR (ip->i4) = OFST ((RE_OPCODE *) u2);
#undef  ip
#define ip  (&(((_tagTemplate *)u1)->U_ORSIGN))
                 ip->i1[0] = I_JMP;     ADDR (ip->i1) = OFST (REip);
#undef  ip
                 u2 = u;
                 u1 = u3;
                 u = * (UINT_PTR *) u1;
                 }
#define ip  (&(t->U_RIGHTOR))
        ip->i1[0] = I_RETURN;
#undef  ip
        return (UINT_PTR) NULL;
        break;

         case CCLBEG:
#define ip  (&(t->U_CCL))
        ip->i1[0] = I_CCL;
        return (UINT_PTR) NULL;

#undef  ip
        break;


         case CCLNOT:
#define ip  (&(t->U_CCL))
        ip->i1[0] = I_NCCL;
        return (UINT_PTR) ip;
#undef  ip
        break;

         case RANGEDBCS1:
#define ip  (&(t->U_RANGE))
        ip->i1[0] = (REPat->fCase || x) ? y : XLTab[y];
        ip->i1[1] = x;
        return (UINT_PTR) ip;
#undef  ip
        break;

         case RANGEDBCS2:
#define ip  ((T_RANGE *)u)
        ip->i1[2] = (REPat->fCase || x) ? y : XLTab[y];
        ip->i1[3] = x;
        return (UINT_PTR) NULL;
#undef  ip
        break;


         case EPILOG:
#define ip  (&(t->U_EPILOG))
        ip->i1[0] = I_MATCH;
        return (UINT_PTR) NULL;
#undef  ip
        break;

         case PREV:
#define ip (&(t->U_PREV))
        ip->i1[0] = I_PREV;
        ip->i1[1] = (RE_OPCODE) u;
        return (UINT_PTR) NULL;
#undef ip

         default:
        InternalErrorBox(SYS_RegExpr_CompileAction);
        return (UINT_PTR) NULL;
        }
}

#if DEBUG
void INTERNAL REDump (p)
struct patType *p;
{
         RE_OPCODE *REip = p->code;

         while (TRUE) {
        printf ("%04x ", REip);
        switch (*REip) {
        case I_CALL:
                 printf ("CALL    %04x\n", ADDR(REip));
                 REip += LCALL;
                 break;
        case I_RETURN:
                 printf ("RETURN\n");
                 REip += LRETURN;
                 break;
        case I_LETTER:
                 printf ("LETTER  %c", REip[1]);
                 REip += LLETTER;
                 break;
        case I_ANY:
                 printf ("ANY\n");
                 REip += LANY;
                 break;
        case I_EOL:
                 printf ("EOL\n");
                 REip += LEOL;
                 break;
        case I_BOL:
                 printf ("BOL\n");
                 REip += LBOL;
                 break;
        case I_CCL:
                 printf ("CCL\n");
                 REip += LCCL;
                 break;
        case I_NCCL:
                 printf ("NCCL\n");
                 REip += LNCCL;
                 break;
        case I_MATCH:
                 printf ("MATCH\n");
                 return;
                 break;
        case I_JMP:
                 printf ("JMP     %04x\n", ADDR(REip));
                 REip += LJMP;
                 break;
        case I_SPTOM:
                 printf ("SPTOM   %04x\n", ADDR(REip));
                 REip += LSPTOM;
                 break;
        case I_PTOM:
                 printf ("PTOM    %04x\n", ADDR(REip));
                 REip += LPTOM;
                 break;
        case I_MTOP:
                 printf ("MTOP    %04x\n", ADDR(REip));
                 REip += LMTOP;
                 break;
        case I_MTOSP:
                 printf ("MTOSP   %04x\n", ADDR(REip));
                 REip += LMTOSP;
                 break;
        case I_FAIL:
                 printf ("FAIL\n");
                 REip += LFAIL;
                 break;
        case I_PUSHP:
                 printf ("PUSHP\n");
                 REip += LPUSHP;
                 break;
        case I_PUSHM:
                 printf ("PUSHM   %04x\n", ADDR(REip));
                 REip += LPUSHM;
                 break;
        case I_POPP:
                 printf ("POPP\n");
                 REip += LPOPP;
                 break;
        case I_POPM:
                 printf ("POPM    %04x\n", ADDR(REip));
                 REip += LPOPM;
                 break;
        case I_PNEQM:
                 printf ("PNEQM   %04x\n", ADDR(REip));
                 REip += LPNEQM;
                 break;
        case I_ITOM:
                 printf ("ITOM    %04x,%04x\n", ADDR(REip), IMM(REip));
                 REip += LITOM;
                 break;
        default:
                 printf ("%04x ???\n", *REip);
                 REip += LOFFSET;
                 return;
                 }
        }
}
#endif

/*  EstimateAction - sum up the number of bytes required by each individual
 *  parsing action in the tree.  Take the input action and add it up to the
 *  running total.
 *
 *  type        type of action being performed
 *  u           dummy parm
 *  x           dummy parm
 *  y           dummy parm
 *
 *  Returns     0 always
 *
 */
UINT_PTR INTERNAL EstimateAction (OPTYPE type, UINT_PTR u,
    unsigned char x, unsigned char y)
{
         u; x; y;

         DEBOUT (("%04x EstimateAction %04x\n", RESize, type));

         if (type < ACTIONMIN || type > ACTIONMAX)
                InternalErrorBox(SYS_RegExpr_EstimateAction);
         RESize += cbIns[type];
         return (UINT_PTR) 0;
}

/*  REEstimate - estimates the number of bytes required to
 *  compile a specified pattern.
 *
 *  REEstimate sets RESize to the number of bytes required to compile
 *  a pattern.  If there is a syntax error in the pattern, RESize is set
 *  to -1.
 *
 *  p           character pointer to pattern that will be compiled
 */
void pascal INTERNAL REEstimate (char *p)
{
         RESize = sizeof (struct patType) - 1;
         REArg = 1;

         EstimateAction (PROLOG, (unsigned int) 0, '\0', '\0');

         if (REParseRE (EstimateAction, p, NULL) == NULL || REArg > MAXPATARG)
        RESize = -1;
         else
        EstimateAction (EPILOG, (unsigned int) 0, '\0', '\0');
}
