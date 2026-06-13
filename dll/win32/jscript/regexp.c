/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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
 */

/*
 * Code in this file is based on files:
 * js/src/jsregexp.h
 * js/src/jsregexp.c
 * from Mozilla project, released under LGPL 2.1 or later.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 */

#include <assert.h>

#include "jscript.h"
#include "regexp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

/* FIXME: Better error handling */
#define ReportRegExpError(a,b,c) throw_error((a)->context, E_FAIL, L"")
#define ReportRegExpErrorHelper(a,b,c,d) throw_error((a)->context, E_FAIL, L"")
#define JS_ReportErrorNumber(a,b,c,d) throw_error((a), E_FAIL, L"")
#define JS_ReportErrorFlagsAndNumber(a,b,c,d,e,f) throw_error((a), E_FAIL, L"")
#define JS_COUNT_OPERATION(a,b) do { } while(0)


typedef BYTE JSPackedBool;

/*
 * This struct holds a bitmap representation of a class from a regexp.
 * There's a list of these referenced by the classList field in the regexp_t
 * struct below. The initial state has startIndex set to the offset in the
 * original regexp source of the beginning of the class contents. The first
 * use of the class converts the source representation into a bitmap.
 *
 */
typedef struct RECharSet {
    JSPackedBool    converted;
    JSPackedBool    sense;
    WORD            length;
    union {
        BYTE        *bits;
        struct {
            size_t  startIndex;
            size_t  length;
        } src;
    } u;
} RECharSet;

#define JSMSG_MIN_TOO_BIG 47
#define JSMSG_MAX_TOO_BIG 48
#define JSMSG_OUT_OF_ORDER 49
#define JSMSG_OUT_OF_MEMORY 137

#define LINE_SEPARATOR  0x2028
#define PARA_SEPARATOR  0x2029

#define RE_IS_LETTER(c)     (((c >= 'A') && (c <= 'Z')) ||                    \
                             ((c >= 'a') && (c <= 'z')) )
#define RE_IS_LINE_TERM(c)  ((c == '\n') || (c == '\r') ||                    \
                             (c == LINE_SEPARATOR) || (c == PARA_SEPARATOR))

#define JS_ISWORD(c)    ((c) < 128 && (isalnum(c) || (c) == '_'))

#define JS7_ISDEC(c)    ((((unsigned)(c)) - '0') <= 9)
#define JS7_UNDEC(c)    ((c) - '0')

typedef enum REOp {
    REOP_EMPTY,
    REOP_BOL,
    REOP_EOL,
    REOP_WBDRY,
    REOP_WNONBDRY,
    REOP_DOT,
    REOP_DIGIT,
    REOP_NONDIGIT,
    REOP_ALNUM,
    REOP_NONALNUM,
    REOP_SPACE,
    REOP_NONSPACE,
    REOP_BACKREF,
    REOP_FLAT,
    REOP_FLAT1,
    REOP_FLATi,
    REOP_FLAT1i,
    REOP_UCFLAT1,
    REOP_UCFLAT1i,
    REOP_UCFLAT,
    REOP_UCFLATi,
    REOP_CLASS,
    REOP_NCLASS,
    REOP_ALT,
    REOP_QUANT,
    REOP_STAR,
    REOP_PLUS,
    REOP_OPT,
    REOP_LPAREN,
    REOP_RPAREN,
    REOP_JUMP,
    REOP_DOTSTAR,
    REOP_LPARENNON,
    REOP_ASSERT,
    REOP_ASSERT_NOT,
    REOP_ASSERTTEST,
    REOP_ASSERTNOTTEST,
    REOP_MINIMALSTAR,
    REOP_MINIMALPLUS,
    REOP_MINIMALOPT,
    REOP_MINIMALQUANT,
    REOP_ENDCHILD,
    REOP_REPEAT,
    REOP_MINIMALREPEAT,
    REOP_ALTPREREQ,
    REOP_ALTPREREQ2,
    REOP_ENDALT,
    REOP_CONCAT,
    REOP_END,
    REOP_LIMIT /* META: no operator >= to this */
} REOp;

#define REOP_IS_SIMPLE(op)  ((op) <= REOP_NCLASS)

static const char *reop_names[] = {
    "empty",
    "bol",
    "eol",
    "wbdry",
    "wnonbdry",
    "dot",
    "digit",
    "nondigit",
    "alnum",
    "nonalnum",
    "space",
    "nonspace",
    "backref",
    "flat",
    "flat1",
    "flati",
    "flat1i",
    "ucflat1",
    "ucflat1i",
    "ucflat",
    "ucflati",
    "class",
    "nclass",
    "alt",
    "quant",
    "star",
    "plus",
    "opt",
    "lparen",
    "rparen",
    "jump",
    "dotstar",
    "lparennon",
    "assert",
    "assert_not",
    "asserttest",
    "assertnottest",
    "minimalstar",
    "minimalplus",
    "minimalopt",
    "minimalquant",
    "endchild",
    "repeat",
    "minimalrepeat",
    "altprereq",
    "alrprereq2",
    "endalt",
    "concat",
    "end",
    NULL
};

typedef struct REProgState {
    jsbytecode *continue_pc;        /* current continuation data */
    jsbytecode continue_op;
    ptrdiff_t index;                /* progress in text */
    size_t parenSoFar;              /* highest indexed paren started */
    union {
        struct {
            UINT min;               /* current quantifier limits */
            UINT max;
        } quantifier;
        struct {
            size_t top;             /* backtrack stack state */
            size_t sz;
        } assertion;
    } u;
} REProgState;

typedef struct REBackTrackData {
    size_t sz;                      /* size of previous stack entry */
    jsbytecode *backtrack_pc;       /* where to backtrack to */
    jsbytecode backtrack_op;
    const WCHAR *cp;                /* index in text of match at backtrack */
    size_t parenIndex;              /* start index of saved paren contents */
    size_t parenCount;              /* # of saved paren contents */
    size_t saveStateStackTop;       /* number of parent states */
    /* saved parent states follow */
    /* saved paren contents follow */
} REBackTrackData;

#define INITIAL_STATESTACK  100
#define INITIAL_BACKTRACK   8000

typedef struct REGlobalData {
    void *cx;
    regexp_t *regexp;               /* the RE in execution */
    BOOL ok;                        /* runtime error (out_of_memory only?) */
    size_t start;                   /* offset to start at */
    ptrdiff_t skipped;              /* chars skipped anchoring this r.e. */
    const WCHAR    *cpbegin;        /* text base address */
    const WCHAR    *cpend;          /* text limit address */

    REProgState *stateStack;        /* stack of state of current parents */
    size_t stateStackTop;
    size_t stateStackLimit;

    REBackTrackData *backTrackStack;/* stack of matched-so-far positions */
    REBackTrackData *backTrackSP;
    size_t backTrackStackSize;
    size_t cursz;                   /* size of current stack entry */
    size_t backTrackCount;          /* how many times we've backtracked */
    size_t backTrackLimit;          /* upper limit on backtrack states */

    heap_pool_t *pool;              /* It's faster to use one malloc'd pool
                                       than to malloc/free the three items
                                       that are allocated from this pool */
} REGlobalData;

typedef struct RENode RENode;
struct RENode {
    REOp            op;         /* r.e. op bytecode */
    RENode          *next;      /* next in concatenation order */
    void            *kid;       /* first operand */
    union {
        void        *kid2;      /* second operand */
        INT         num;        /* could be a number */
        size_t      parenIndex; /* or a parenthesis index */
        struct {                /* or a quantifier range */
            UINT  min;
            UINT  max;
            JSPackedBool greedy;
        } range;
        struct {                /* or a character class */
            size_t  startIndex;
            size_t  kidlen;     /* length of string at kid, in jschars */
            size_t  index;      /* index into class list */
            WORD  bmsize;       /* bitmap size, based on max char code */
            JSPackedBool sense;
        } ucclass;
        struct {                /* or a literal sequence */
            WCHAR   chr;        /* of one character */
            size_t  length;     /* or many (via the kid) */
        } flat;
        struct {
            RENode  *kid2;      /* second operand from ALT */
            WCHAR   ch1;        /* match char for ALTPREREQ */
            WCHAR   ch2;        /* ditto, or class index for ALTPREREQ2 */
        } altprereq;
    } u;
};

#define CLASS_CACHE_SIZE    4

typedef struct CompilerState {
    script_ctx_t    *context;
    const WCHAR     *cpbegin;
    const WCHAR     *cpend;
    const WCHAR     *cp;
    size_t          parenCount;
    size_t          classCount;   /* number of [] encountered */
    size_t          treeDepth;    /* maximum depth of parse tree */
    size_t          progLength;   /* estimated bytecode length */
    RENode          *result;
    size_t          classBitmapsMem; /* memory to hold all class bitmaps */
    struct {
        const WCHAR *start;         /* small cache of class strings */
        size_t length;              /* since they're often the same */
        size_t index;
    } classCache[CLASS_CACHE_SIZE];
    WORD          flags;

    heap_pool_t *pool;              /* It's faster to use one malloc'd pool
                                       than to malloc/free */
} CompilerState;

typedef struct EmitStateStackEntry {
    jsbytecode      *altHead;       /* start of REOP_ALT* opcode */
    jsbytecode      *nextAltFixup;  /* fixup pointer to next-alt offset */
    jsbytecode      *nextTermFixup; /* fixup ptr. to REOP_JUMP offset */
    jsbytecode      *endTermFixup;  /* fixup ptr. to REOPT_ALTPREREQ* offset */
    RENode          *continueNode;  /* original REOP_ALT* node being stacked */
    jsbytecode      continueOp;     /* REOP_JUMP or REOP_ENDALT continuation */
    JSPackedBool    jumpToJumpFlag; /* true if we've patched jump-to-jump to
                                       avoid 16-bit unsigned offset overflow */
} EmitStateStackEntry;

/*
 * Immediate operand sizes and getter/setters.  Unlike the ones in jsopcode.h,
 * the getters and setters take the pc of the offset, not of the opcode before
 * the offset.
 */
#define ARG_LEN             2
#define GET_ARG(pc)         ((WORD)(((pc)[0] << 8) | (pc)[1]))
#define SET_ARG(pc, arg)    ((pc)[0] = (jsbytecode) ((arg) >> 8),       \
                             (pc)[1] = (jsbytecode) (arg))

#define OFFSET_LEN          ARG_LEN
#define OFFSET_MAX          ((1 << (ARG_LEN * 8)) - 1)
#define GET_OFFSET(pc)      GET_ARG(pc)

static BOOL ParseRegExp(CompilerState*);

/*
 * Maximum supported tree depth is maximum size of EmitStateStackEntry stack.
 * For sanity, we limit it to 2^24 bytes.
 */
#define TREE_DEPTH_MAX  ((1 << 24) / sizeof(EmitStateStackEntry))

/*
 * The maximum memory that can be allocated for class bitmaps.
 * For sanity, we limit it to 2^24 bytes.
 */
#define CLASS_BITMAPS_MEM_LIMIT (1 << 24)

/*
 * Functions to get size and write/read bytecode that represent small indexes
 * compactly.
 * Each byte in the code represent 7-bit chunk of the index. 8th bit when set
 * indicates that the following byte brings more bits to the index. Otherwise
 * this is the last byte in the index bytecode representing highest index bits.
 */
static size_t
GetCompactIndexWidth(size_t index)
{
    size_t width;

    for (width = 1; (index >>= 7) != 0; ++width) { }
    return width;
}

static inline jsbytecode *
WriteCompactIndex(jsbytecode *pc, size_t index)
{
    size_t next;

    while ((next = index >> 7) != 0) {
        *pc++ = (jsbytecode)(index | 0x80);
        index = next;
    }
    *pc++ = (jsbytecode)index;
    return pc;
}

static inline jsbytecode *
ReadCompactIndex(jsbytecode *pc, size_t *result)
{
    size_t nextByte;

    nextByte = *pc++;
    if ((nextByte & 0x80) == 0) {
        /*
         * Short-circuit the most common case when compact index <= 127.
         */
        *result = nextByte;
    } else {
        size_t shift = 7;
        *result = 0x7F & nextByte;
        do {
            nextByte = *pc++;
            *result |= (nextByte & 0x7F) << shift;
            shift += 7;
        } while ((nextByte & 0x80) != 0);
    }
    return pc;
}

/* Construct and initialize an RENode, returning NULL for out-of-memory */
static RENode *
NewRENode(CompilerState *state, REOp op)
{
    RENode *ren;

    ren = heap_pool_alloc(state->pool, sizeof(*ren));
    if (!ren) {
        throw_error(state->context, E_OUTOFMEMORY, L"");
        return NULL;
    }
    ren->op = op;
    ren->next = NULL;
    ren->kid = NULL;
    return ren;
}

/*
 * Validates and converts hex ascii value.
 */
static BOOL
isASCIIHexDigit(WCHAR c, UINT *digit)
{
    UINT cv = c;

    if (cv < '0')
        return FALSE;
    if (cv <= '9') {
        *digit = cv - '0';
        return TRUE;
    }
    cv |= 0x20;
    if (cv >= 'a' && cv <= 'f') {
        *digit = cv - 'a' + 10;
        return TRUE;
    }
    return FALSE;
}

typedef struct {
    REOp op;
    const WCHAR *errPos;
    size_t parenIndex;
} REOpData;

#define JUMP_OFFSET_HI(off)     ((jsbytecode)((off) >> 8))
#define JUMP_OFFSET_LO(off)     ((jsbytecode)(off))

static BOOL
SetForwardJumpOffset(jsbytecode *jump, jsbytecode *target)
{
    ptrdiff_t offset = target - jump;

    /* Check that target really points forward. */
    assert(offset >= 2);
    if ((size_t)offset > OFFSET_MAX)
        return FALSE;

    jump[0] = JUMP_OFFSET_HI(offset);
    jump[1] = JUMP_OFFSET_LO(offset);
    return TRUE;
}

/*
 * Generate bytecode for the tree rooted at t using an explicit stack instead
 * of recursion.
 */
static jsbytecode *
EmitREBytecode(CompilerState *state, regexp_t *re, size_t treeDepth,
               jsbytecode *pc, RENode *t)
{
    EmitStateStackEntry *emitStateSP, *emitStateStack;
    RECharSet *charSet;
    REOp op;

    if (treeDepth == 0) {
        emitStateStack = NULL;
    } else {
        emitStateStack = malloc(sizeof(EmitStateStackEntry) * treeDepth);
        if (!emitStateStack)
            return NULL;
    }
    emitStateSP = emitStateStack;
    op = t->op;
    assert(op < REOP_LIMIT);

    for (;;) {
        *pc++ = op;
        switch (op) {
          case REOP_EMPTY:
            --pc;
            break;

          case REOP_ALTPREREQ2:
          case REOP_ALTPREREQ:
            assert(emitStateSP);
            emitStateSP->altHead = pc - 1;
            emitStateSP->endTermFixup = pc;
            pc += OFFSET_LEN;
            SET_ARG(pc, t->u.altprereq.ch1);
            pc += ARG_LEN;
            SET_ARG(pc, t->u.altprereq.ch2);
            pc += ARG_LEN;

            emitStateSP->nextAltFixup = pc;    /* offset to next alternate */
            pc += OFFSET_LEN;

            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_JUMP;
            emitStateSP->jumpToJumpFlag = FALSE;
            ++emitStateSP;
            assert((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->kid;
            op = t->op;
            assert(op < REOP_LIMIT);
            continue;

          case REOP_JUMP:
            emitStateSP->nextTermFixup = pc;    /* offset to following term */
            pc += OFFSET_LEN;
            if (!SetForwardJumpOffset(emitStateSP->nextAltFixup, pc))
                goto jump_too_big;
            emitStateSP->continueOp = REOP_ENDALT;
            ++emitStateSP;
            assert((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->u.kid2;
            op = t->op;
            assert(op < REOP_LIMIT);
            continue;

          case REOP_ENDALT:
            /*
             * If we already patched emitStateSP->nextTermFixup to jump to
             * a nearer jump, to avoid 16-bit immediate offset overflow, we
             * are done here.
             */
            if (emitStateSP->jumpToJumpFlag)
                break;

            /*
             * Fix up the REOP_JUMP offset to go to the op after REOP_ENDALT.
             * REOP_ENDALT is executed only on successful match of the last
             * alternate in a group.
             */
            if (!SetForwardJumpOffset(emitStateSP->nextTermFixup, pc))
                goto jump_too_big;
            if (t->op != REOP_ALT) {
                if (!SetForwardJumpOffset(emitStateSP->endTermFixup, pc))
                    goto jump_too_big;
            }

            /*
             * If the program is bigger than the REOP_JUMP offset range, then
             * we must check for alternates before this one that are part of
             * the same group, and fix up their jump offsets to target jumps
             * close enough to fit in a 16-bit unsigned offset immediate.
             */
            if ((size_t)(pc - re->program) > OFFSET_MAX &&
                emitStateSP > emitStateStack) {
                EmitStateStackEntry *esp, *esp2;
                jsbytecode *alt, *jump;
                ptrdiff_t span, header;

                esp2 = emitStateSP;
                alt = esp2->altHead;
                for (esp = esp2 - 1; esp >= emitStateStack; --esp) {
                    if (esp->continueOp == REOP_ENDALT &&
                        !esp->jumpToJumpFlag &&
                        esp->nextTermFixup + OFFSET_LEN == alt &&
                        (size_t)(pc - ((esp->continueNode->op != REOP_ALT)
                                       ? esp->endTermFixup
                                       : esp->nextTermFixup)) > OFFSET_MAX) {
                        alt = esp->altHead;
                        jump = esp->nextTermFixup;

                        /*
                         * The span must be 1 less than the distance from
                         * jump offset to jump offset, so we actually jump
                         * to a REOP_JUMP bytecode, not to its offset!
                         */
                        for (;;) {
                            assert(jump < esp2->nextTermFixup);
                            span = esp2->nextTermFixup - jump - 1;
                            if ((size_t)span <= OFFSET_MAX)
                                break;
                            do {
                                if (--esp2 == esp)
                                    goto jump_too_big;
                            } while (esp2->continueOp != REOP_ENDALT);
                        }

                        jump[0] = JUMP_OFFSET_HI(span);
                        jump[1] = JUMP_OFFSET_LO(span);

                        if (esp->continueNode->op != REOP_ALT) {
                            /*
                             * We must patch the offset at esp->endTermFixup
                             * as well, for the REOP_ALTPREREQ{,2} opcodes.
                             * If we're unlucky and endTermFixup is more than
                             * OFFSET_MAX bytes from its target, we cheat by
                             * jumping 6 bytes to the jump whose offset is at
                             * esp->nextTermFixup, which has the same target.
                             */
                            jump = esp->endTermFixup;
                            header = esp->nextTermFixup - jump;
                            span += header;
                            if ((size_t)span > OFFSET_MAX)
                                span = header;

                            jump[0] = JUMP_OFFSET_HI(span);
                            jump[1] = JUMP_OFFSET_LO(span);
                        }

                        esp->jumpToJumpFlag = TRUE;
                    }
                }
            }
            break;

          case REOP_ALT:
            assert(emitStateSP);
            emitStateSP->altHead = pc - 1;
            emitStateSP->nextAltFixup = pc;     /* offset to next alternate */
            pc += OFFSET_LEN;
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_JUMP;
            emitStateSP->jumpToJumpFlag = FALSE;
            ++emitStateSP;
            assert((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->kid;
            op = t->op;
            assert(op < REOP_LIMIT);
            continue;

          case REOP_FLAT:
            /*
             * Coalesce FLATs if possible and if it would not increase bytecode
             * beyond preallocated limit. The latter happens only when bytecode
             * size for coalesced string with offset p and length 2 exceeds 6
             * bytes preallocated for 2 single char nodes, i.e. when
             * 1 + GetCompactIndexWidth(p) + GetCompactIndexWidth(2) > 6 or
             * GetCompactIndexWidth(p) > 4.
             * Since when GetCompactIndexWidth(p) <= 4 coalescing of 3 or more
             * nodes strictly decreases bytecode size, the check has to be
             * done only for the first coalescing.
             */
            if (t->kid &&
                GetCompactIndexWidth((WCHAR*)t->kid - state->cpbegin) <= 4)
            {
                while (t->next &&
                       t->next->op == REOP_FLAT &&
                       (WCHAR*)t->kid + t->u.flat.length ==
                       t->next->kid) {
                    t->u.flat.length += t->next->u.flat.length;
                    t->next = t->next->next;
                }
            }
            if (t->kid && t->u.flat.length > 1) {
                pc[-1] = (state->flags & REG_FOLD) ? REOP_FLATi : REOP_FLAT;
                pc = WriteCompactIndex(pc, (WCHAR*)t->kid - state->cpbegin);
                pc = WriteCompactIndex(pc, t->u.flat.length);
            } else if (t->u.flat.chr < 256) {
                pc[-1] = (state->flags & REG_FOLD) ? REOP_FLAT1i : REOP_FLAT1;
                *pc++ = (jsbytecode) t->u.flat.chr;
            } else {
                pc[-1] = (state->flags & REG_FOLD)
                         ? REOP_UCFLAT1i
                         : REOP_UCFLAT1;
                SET_ARG(pc, t->u.flat.chr);
                pc += ARG_LEN;
            }
            break;

          case REOP_LPAREN:
            assert(emitStateSP);
            pc = WriteCompactIndex(pc, t->u.parenIndex);
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_RPAREN;
            ++emitStateSP;
            assert((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->kid;
            op = t->op;
            continue;

          case REOP_RPAREN:
            pc = WriteCompactIndex(pc, t->u.parenIndex);
            break;

          case REOP_BACKREF:
            pc = WriteCompactIndex(pc, t->u.parenIndex);
            break;

          case REOP_ASSERT:
            assert(emitStateSP);
            emitStateSP->nextTermFixup = pc;
            pc += OFFSET_LEN;
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_ASSERTTEST;
            ++emitStateSP;
            assert((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->kid;
            op = t->op;
            continue;

          case REOP_ASSERTTEST:
          case REOP_ASSERTNOTTEST:
            if (!SetForwardJumpOffset(emitStateSP->nextTermFixup, pc))
                goto jump_too_big;
            break;

          case REOP_ASSERT_NOT:
            assert(emitStateSP);
            emitStateSP->nextTermFixup = pc;
            pc += OFFSET_LEN;
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_ASSERTNOTTEST;
            ++emitStateSP;
            assert((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->kid;
            op = t->op;
            continue;

          case REOP_QUANT:
            assert(emitStateSP);
            if (t->u.range.min == 0 && t->u.range.max == (UINT)-1) {
                pc[-1] = (t->u.range.greedy) ? REOP_STAR : REOP_MINIMALSTAR;
            } else if (t->u.range.min == 0 && t->u.range.max == 1) {
                pc[-1] = (t->u.range.greedy) ? REOP_OPT : REOP_MINIMALOPT;
            } else if (t->u.range.min == 1 && t->u.range.max == (UINT) -1) {
                pc[-1] = (t->u.range.greedy) ? REOP_PLUS : REOP_MINIMALPLUS;
            } else {
                if (!t->u.range.greedy)
                    pc[-1] = REOP_MINIMALQUANT;
                pc = WriteCompactIndex(pc, t->u.range.min);
                /*
                 * Write max + 1 to avoid using size_t(max) + 1 bytes
                 * for (UINT)-1 sentinel.
                 */
                pc = WriteCompactIndex(pc, t->u.range.max + 1);
            }
            emitStateSP->nextTermFixup = pc;
            pc += OFFSET_LEN;
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_ENDCHILD;
            ++emitStateSP;
            assert((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->kid;
            op = t->op;
            continue;

          case REOP_ENDCHILD:
            if (!SetForwardJumpOffset(emitStateSP->nextTermFixup, pc))
                goto jump_too_big;
            break;

          case REOP_CLASS:
            if (!t->u.ucclass.sense)
                pc[-1] = REOP_NCLASS;
            pc = WriteCompactIndex(pc, t->u.ucclass.index);
            charSet = &re->classList[t->u.ucclass.index];
            charSet->converted = FALSE;
            charSet->length = t->u.ucclass.bmsize;
            charSet->u.src.startIndex = t->u.ucclass.startIndex;
            charSet->u.src.length = t->u.ucclass.kidlen;
            charSet->sense = t->u.ucclass.sense;
            break;

          default:
            break;
        }

        t = t->next;
        if (t) {
            op = t->op;
        } else {
            if (emitStateSP == emitStateStack)
                break;
            --emitStateSP;
            t = emitStateSP->continueNode;
            op = (REOp) emitStateSP->continueOp;
        }
    }

  cleanup:
    free(emitStateStack);
    return pc;

  jump_too_big:
    ReportRegExpError(state, JSREPORT_ERROR, JSMSG_REGEXP_TOO_COMPLEX);
    pc = NULL;
    goto cleanup;
}

/*
 * Process the op against the two top operands, reducing them to a single
 * operand in the penultimate slot. Update progLength and treeDepth.
 */
static BOOL
ProcessOp(CompilerState *state, REOpData *opData, RENode **operandStack,
          INT operandSP)
{
    RENode *result;

    switch (opData->op) {
      case REOP_ALT:
        result = NewRENode(state, REOP_ALT);
        if (!result)
            return FALSE;
        result->kid = operandStack[operandSP - 2];
        result->u.kid2 = operandStack[operandSP - 1];
        operandStack[operandSP - 2] = result;

        if (state->treeDepth == TREE_DEPTH_MAX) {
            ReportRegExpError(state, JSREPORT_ERROR, JSMSG_REGEXP_TOO_COMPLEX);
            return FALSE;
        }
        ++state->treeDepth;

        /*
         * Look at both alternates to see if there's a FLAT or a CLASS at
         * the start of each. If so, use a prerequisite match.
         */
        if (((RENode *) result->kid)->op == REOP_FLAT &&
            ((RENode *) result->u.kid2)->op == REOP_FLAT &&
            (state->flags & REG_FOLD) == 0) {
            result->op = REOP_ALTPREREQ;
            result->u.altprereq.ch1 = ((RENode *) result->kid)->u.flat.chr;
            result->u.altprereq.ch2 = ((RENode *) result->u.kid2)->u.flat.chr;
            /* ALTPREREQ, <end>, uch1, uch2, <next>, ...,
                                            JUMP, <end> ... ENDALT */
            state->progLength += 13;
        }
        else
        if (((RENode *) result->kid)->op == REOP_CLASS &&
            ((RENode *) result->kid)->u.ucclass.index < 256 &&
            ((RENode *) result->u.kid2)->op == REOP_FLAT &&
            (state->flags & REG_FOLD) == 0) {
            result->op = REOP_ALTPREREQ2;
            result->u.altprereq.ch1 = ((RENode *) result->u.kid2)->u.flat.chr;
            result->u.altprereq.ch2 = ((RENode *) result->kid)->u.ucclass.index;
            /* ALTPREREQ2, <end>, uch1, uch2, <next>, ...,
                                            JUMP, <end> ... ENDALT */
            state->progLength += 13;
        }
        else
        if (((RENode *) result->kid)->op == REOP_FLAT &&
            ((RENode *) result->u.kid2)->op == REOP_CLASS &&
            ((RENode *) result->u.kid2)->u.ucclass.index < 256 &&
            (state->flags & REG_FOLD) == 0) {
            result->op = REOP_ALTPREREQ2;
            result->u.altprereq.ch1 = ((RENode *) result->kid)->u.flat.chr;
            result->u.altprereq.ch2 =
                ((RENode *) result->u.kid2)->u.ucclass.index;
            /* ALTPREREQ2, <end>, uch1, uch2, <next>, ...,
                                          JUMP, <end> ... ENDALT */
            state->progLength += 13;
        }
        else {
            /* ALT, <next>, ..., JUMP, <end> ... ENDALT */
            state->progLength += 7;
        }
        break;

      case REOP_CONCAT:
        result = operandStack[operandSP - 2];
        while (result->next)
            result = result->next;
        result->next = operandStack[operandSP - 1];
        break;

      case REOP_ASSERT:
      case REOP_ASSERT_NOT:
      case REOP_LPARENNON:
      case REOP_LPAREN:
        /* These should have been processed by a close paren. */
        ReportRegExpErrorHelper(state, JSREPORT_ERROR, JSMSG_MISSING_PAREN,
                                opData->errPos);
        return FALSE;

      default:;
    }
    return TRUE;
}

/*
 * Hack two bits in CompilerState.flags, for use within FindParenCount to flag
 * it being on the stack, and to propagate errors to its callers.
 */
#define JSREG_FIND_PAREN_COUNT  0x8000
#define JSREG_FIND_PAREN_ERROR  0x4000

/*
 * Magic return value from FindParenCount and GetDecimalValue, to indicate
 * overflow beyond GetDecimalValue's max parameter, or a computed maximum if
 * its findMax parameter is non-null.
 */
#define OVERFLOW_VALUE          ((UINT)-1)

static UINT
FindParenCount(CompilerState *state)
{
    CompilerState temp;
    int i;

    if (state->flags & JSREG_FIND_PAREN_COUNT)
        return OVERFLOW_VALUE;

    /*
     * Copy state into temp, flag it so we never report an invalid backref,
     * and reset its members to parse the entire regexp.  This is obviously
     * suboptimal, but GetDecimalValue calls us only if a backref appears to
     * refer to a forward parenthetical, which is rare.
     */
    temp = *state;
    temp.flags |= JSREG_FIND_PAREN_COUNT;
    temp.cp = temp.cpbegin;
    temp.parenCount = 0;
    temp.classCount = 0;
    temp.progLength = 0;
    temp.treeDepth = 0;
    temp.classBitmapsMem = 0;
    for (i = 0; i < CLASS_CACHE_SIZE; i++)
        temp.classCache[i].start = NULL;

    if (!ParseRegExp(&temp)) {
        state->flags |= JSREG_FIND_PAREN_ERROR;
        return OVERFLOW_VALUE;
    }
    return temp.parenCount;
}

/*
 * Extract and return a decimal value at state->cp.  The initial character c
 * has already been read.  Return OVERFLOW_VALUE if the result exceeds max.
 * Callers who pass a non-null findMax should test JSREG_FIND_PAREN_ERROR in
 * state->flags to discover whether an error occurred under findMax.
 */
static UINT
GetDecimalValue(WCHAR c, UINT max, UINT (*findMax)(CompilerState *state),
                CompilerState *state)
{
    UINT value = JS7_UNDEC(c);
    BOOL overflow = (value > max && (!findMax || value > findMax(state)));

    /* The following restriction allows simpler overflow checks. */
    assert(max <= ((UINT)-1 - 9) / 10);
    while (state->cp < state->cpend) {
        c = *state->cp;
        if (!JS7_ISDEC(c))
            break;
        value = 10 * value + JS7_UNDEC(c);
        if (!overflow && value > max && (!findMax || value > findMax(state)))
            overflow = TRUE;
        ++state->cp;
    }
    return overflow ? OVERFLOW_VALUE : value;
}

/*
 * Calculate the total size of the bitmap required for a class expression.
 */
static BOOL
CalculateBitmapSize(CompilerState *state, RENode *target, const WCHAR *src,
                    const WCHAR *end)
{
    UINT max = 0;
    BOOL inRange = FALSE;
    WCHAR c, rangeStart = 0;
    UINT n, digit, nDigits, i;

    target->u.ucclass.bmsize = 0;
    target->u.ucclass.sense = TRUE;

    if (src == end)
        return TRUE;

    if (*src == '^') {
        ++src;
        target->u.ucclass.sense = FALSE;
    }

    while (src != end) {
        BOOL canStartRange = TRUE;
        UINT localMax = 0;

        switch (*src) {
          case '\\':
            ++src;
            c = *src++;
            switch (c) {
              case 'b':
                localMax = 0x8;
                break;
              case 'f':
                localMax = 0xC;
                break;
              case 'n':
                localMax = 0xA;
                break;
              case 'r':
                localMax = 0xD;
                break;
              case 't':
                localMax = 0x9;
                break;
              case 'v':
                localMax = 0xB;
                break;
              case 'c':
                if (src < end && RE_IS_LETTER(*src)) {
                    localMax = (UINT) (*src++) & 0x1F;
                } else {
                    --src;
                    localMax = '\\';
                }
                break;
              case 'x':
                nDigits = 2;
                goto lexHex;
              case 'u':
                nDigits = 4;
lexHex:
                n = 0;
                for (i = 0; (i < nDigits) && (src < end); i++) {
                    c = *src++;
                    if (!isASCIIHexDigit(c, &digit)) {
                        /*
                         * Back off to accepting the original
                         *'\' as a literal.
                         */
                        src -= i + 1;
                        n = '\\';
                        break;
                    }
                    n = (n << 4) | digit;
                }
                localMax = n;
                break;
              case 'd':
                canStartRange = FALSE;
                if (inRange) {
                    JS_ReportErrorNumber(state->context,
                                         js_GetErrorMessage, NULL,
                                         JSMSG_BAD_CLASS_RANGE);
                    return FALSE;
                }
                localMax = '9';
                break;
              case 'D':
              case 's':
              case 'S':
              case 'w':
              case 'W':
                canStartRange = FALSE;
                if (inRange) {
                    JS_ReportErrorNumber(state->context,
                                         js_GetErrorMessage, NULL,
                                         JSMSG_BAD_CLASS_RANGE);
                    return FALSE;
                }
                max = 65535;

                /*
                 * If this is the start of a range, ensure that it's less than
                 * the end.
                 */
                localMax = 0;
                break;
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
                /*
                 *  This is a non-ECMA extension - decimal escapes (in this
                 *  case, octal!) are supposed to be an error inside class
                 *  ranges, but supported here for backwards compatibility.
                 *
                 */
                n = JS7_UNDEC(c);
                c = *src;
                if ('0' <= c && c <= '7') {
                    src++;
                    n = 8 * n + JS7_UNDEC(c);
                    c = *src;
                    if ('0' <= c && c <= '7') {
                        src++;
                        i = 8 * n + JS7_UNDEC(c);
                        if (i <= 0377)
                            n = i;
                        else
                            src--;
                    }
                }
                localMax = n;
                break;

              default:
                localMax = c;
                break;
            }
            break;
          default:
            localMax = *src++;
            break;
        }

        if (inRange) {
            /* Throw a SyntaxError here, per ECMA-262, 15.10.2.15. */
            if (rangeStart > localMax) {
                JS_ReportErrorNumber(state->context,
                                     js_GetErrorMessage, NULL,
                                     JSMSG_BAD_CLASS_RANGE);
                return FALSE;
            }
            inRange = FALSE;
        } else {
            if (canStartRange && src < end - 1) {
                if (*src == '-') {
                    ++src;
                    inRange = TRUE;
                    rangeStart = (WCHAR)localMax;
                    continue;
                }
            }
            if (state->flags & REG_FOLD)
                rangeStart = localMax;   /* one run of the uc/dc loop below */
        }

        if (state->flags & REG_FOLD) {
            WCHAR maxch = localMax;

            for (i = rangeStart; i <= localMax; i++) {
                WCHAR uch, dch;

                uch = towupper(i);
                dch = towlower(i);
                if(maxch < uch)
                    maxch = uch;
                if(maxch < dch)
                    maxch = dch;
            }
            localMax = maxch;
        }

        if (localMax > max)
            max = localMax;
    }
    target->u.ucclass.bmsize = max;
    return TRUE;
}

static INT
ParseMinMaxQuantifier(CompilerState *state, BOOL ignoreValues)
{
    UINT min, max;
    WCHAR c;
    const WCHAR *errp = state->cp++;

    c = *state->cp;
    if (JS7_ISDEC(c)) {
        ++state->cp;
        min = GetDecimalValue(c, 0xFFFF, NULL, state);
        c = *state->cp;

        if (!ignoreValues && min == OVERFLOW_VALUE)
            return JSMSG_MIN_TOO_BIG;

        if (c == ',') {
            c = *++state->cp;
            if (JS7_ISDEC(c)) {
                ++state->cp;
                max = GetDecimalValue(c, 0xFFFF, NULL, state);
                c = *state->cp;
                if (!ignoreValues && max == OVERFLOW_VALUE)
                    return JSMSG_MAX_TOO_BIG;
                if (!ignoreValues && min > max)
                    return JSMSG_OUT_OF_ORDER;
            } else {
                max = (UINT)-1;
            }
        } else {
            max = min;
        }
        if (c == '}') {
            state->result = NewRENode(state, REOP_QUANT);
            if (!state->result)
                return JSMSG_OUT_OF_MEMORY;
            state->result->u.range.min = min;
            state->result->u.range.max = max;
            /*
             * QUANT, <min>, <max>, <next> ... <ENDCHILD>
             * where <max> is written as compact(max+1) to make
             * (UINT)-1 sentinel to occupy 1 byte, not width_of(max)+1.
             */
            state->progLength += (1 + GetCompactIndexWidth(min)
                                  + GetCompactIndexWidth(max + 1)
                                  +3);
            return 0;
        }
    }

    state->cp = errp;
    return -1;
}

static BOOL
ParseQuantifier(CompilerState *state)
{
    RENode *term;
    term = state->result;
    if (state->cp < state->cpend) {
        switch (*state->cp) {
          case '+':
            state->result = NewRENode(state, REOP_QUANT);
            if (!state->result)
                return FALSE;
            state->result->u.range.min = 1;
            state->result->u.range.max = (UINT)-1;
            /* <PLUS>, <next> ... <ENDCHILD> */
            state->progLength += 4;
            goto quantifier;
          case '*':
            state->result = NewRENode(state, REOP_QUANT);
            if (!state->result)
                return FALSE;
            state->result->u.range.min = 0;
            state->result->u.range.max = (UINT)-1;
            /* <STAR>, <next> ... <ENDCHILD> */
            state->progLength += 4;
            goto quantifier;
          case '?':
            state->result = NewRENode(state, REOP_QUANT);
            if (!state->result)
                return FALSE;
            state->result->u.range.min = 0;
            state->result->u.range.max = 1;
            /* <OPT>, <next> ... <ENDCHILD> */
            state->progLength += 4;
            goto quantifier;
          case '{':       /* balance '}' */
          {
            INT err;

            err = ParseMinMaxQuantifier(state, FALSE);
            if (err == 0)
                goto quantifier;
            if (err == -1)
                return TRUE;

            ReportRegExpErrorHelper(state, JSREPORT_ERROR, err, errp);
            return FALSE;
          }
          default:;
        }
    }
    return TRUE;

quantifier:
    if (state->treeDepth == TREE_DEPTH_MAX) {
        ReportRegExpError(state, JSREPORT_ERROR, JSMSG_REGEXP_TOO_COMPLEX);
        return FALSE;
    }

    ++state->treeDepth;
    ++state->cp;
    state->result->kid = term;
    if (state->cp < state->cpend && *state->cp == '?') {
        ++state->cp;
        state->result->u.range.greedy = FALSE;
    } else {
        state->result->u.range.greedy = TRUE;
    }
    return TRUE;
}

/*
 *  item:       assertion               An item is either an assertion or
 *              quantatom               a quantified atom.
 *
 *  assertion:  '^'                     Assertions match beginning of string
 *                                      (or line if the class static property
 *                                      RegExp.multiline is true).
 *              '$'                     End of string (or line if the class
 *                                      static property RegExp.multiline is
 *                                      true).
 *              '\b'                    Word boundary (between \w and \W).
 *              '\B'                    Word non-boundary.
 *
 *  quantatom:  atom                    An unquantified atom.
 *              quantatom '{' n ',' m '}'
 *                                      Atom must occur between n and m times.
 *              quantatom '{' n ',' '}' Atom must occur at least n times.
 *              quantatom '{' n '}'     Atom must occur exactly n times.
 *              quantatom '*'           Zero or more times (same as {0,}).
 *              quantatom '+'           One or more times (same as {1,}).
 *              quantatom '?'           Zero or one time (same as {0,1}).
 *
 *              any of which can be optionally followed by '?' for ungreedy
 *
 *  atom:       '(' regexp ')'          A parenthesized regexp (what matched
 *                                      can be addressed using a backreference,
 *                                      see '\' n below).
 *              '.'                     Matches any char except '\n'.
 *              '[' classlist ']'       A character class.
 *              '[' '^' classlist ']'   A negated character class.
 *              '\f'                    Form Feed.
 *              '\n'                    Newline (Line Feed).
 *              '\r'                    Carriage Return.
 *              '\t'                    Horizontal Tab.
 *              '\v'                    Vertical Tab.
 *              '\d'                    A digit (same as [0-9]).
 *              '\D'                    A non-digit.
 *              '\w'                    A word character, [0-9a-z_A-Z].
 *              '\W'                    A non-word character.
 *              '\s'                    A whitespace character, [ \b\f\n\r\t\v].
 *              '\S'                    A non-whitespace character.
 *              '\' n                   A backreference to the nth (n decimal
 *                                      and positive) parenthesized expression.
 *              '\' octal               An octal escape sequence (octal must be
 *                                      two or three digits long, unless it is
 *                                      0 for the null character).
 *              '\x' hex                A hex escape (hex must be two digits).
 *              '\u' unicode            A unicode escape (must be four digits).
 *              '\c' ctrl               A control character, ctrl is a letter.
 *              '\' literalatomchar     Any character except one of the above
 *                                      that follow '\' in an atom.
 *              otheratomchar           Any character not first among the other
 *                                      atom right-hand sides.
 */
static BOOL
ParseTerm(CompilerState *state)
{
    WCHAR c = *state->cp++;
    UINT nDigits;
    UINT num, tmp, n, i;
    const WCHAR *termStart;

    switch (c) {
    /* assertions and atoms */
      case '^':
        state->result = NewRENode(state, REOP_BOL);
        if (!state->result)
            return FALSE;
        state->progLength++;
        return TRUE;
      case '$':
        state->result = NewRENode(state, REOP_EOL);
        if (!state->result)
            return FALSE;
        state->progLength++;
        return TRUE;
      case '\\':
        if (state->cp >= state->cpend) {
            /* a trailing '\' is an error */
            ReportRegExpError(state, JSREPORT_ERROR, JSMSG_TRAILING_SLASH);
            return FALSE;
        }
        c = *state->cp++;
        switch (c) {
        /* assertion escapes */
          case 'b' :
            state->result = NewRENode(state, REOP_WBDRY);
            if (!state->result)
                return FALSE;
            state->progLength++;
            return TRUE;
          case 'B':
            state->result = NewRENode(state, REOP_WNONBDRY);
            if (!state->result)
                return FALSE;
            state->progLength++;
            return TRUE;
          /* Decimal escape */
          case '0':
              /* Give a strict warning. See also the note below. */
              WARN("non-octal digit in an escape sequence that doesn't match a back-reference\n");
     doOctal:
            num = 0;
            while (state->cp < state->cpend) {
                c = *state->cp;
                if (c < '0' || '7' < c)
                    break;
                state->cp++;
                tmp = 8 * num + (UINT)JS7_UNDEC(c);
                if (tmp > 0377)
                    break;
                num = tmp;
            }
            c = (WCHAR)num;
    doFlat:
            state->result = NewRENode(state, REOP_FLAT);
            if (!state->result)
                return FALSE;
            state->result->u.flat.chr = c;
            state->result->u.flat.length = 1;
            state->progLength += 3;
            break;
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            termStart = state->cp - 1;
            num = GetDecimalValue(c, state->parenCount, FindParenCount, state);
            if (state->flags & JSREG_FIND_PAREN_ERROR)
                return FALSE;
            if (num == OVERFLOW_VALUE) {
                /* Give a strict mode warning. */
                WARN("back-reference exceeds number of capturing parentheses\n");

                /*
                 * Note: ECMA 262, 15.10.2.9 says that we should throw a syntax
                 * error here. However, for compatibility with IE, we treat the
                 * whole backref as flat if the first character in it is not a
                 * valid octal character, and as an octal escape otherwise.
                 */
                state->cp = termStart;
                if (c >= '8') {
                    /* Treat this as flat. termStart - 1 is the \. */
                    c = '\\';
                    goto asFlat;
                }

                /* Treat this as an octal escape. */
                goto doOctal;
            }
            assert(1 <= num && num <= 0x10000);
            state->result = NewRENode(state, REOP_BACKREF);
            if (!state->result)
                return FALSE;
            state->result->u.parenIndex = num - 1;
            state->progLength
                += 1 + GetCompactIndexWidth(state->result->u.parenIndex);
            break;
          /* Control escape */
          case 'f':
            c = 0xC;
            goto doFlat;
          case 'n':
            c = 0xA;
            goto doFlat;
          case 'r':
            c = 0xD;
            goto doFlat;
          case 't':
            c = 0x9;
            goto doFlat;
          case 'v':
            c = 0xB;
            goto doFlat;
          /* Control letter */
          case 'c':
            if (state->cp < state->cpend && RE_IS_LETTER(*state->cp)) {
                c = (WCHAR) (*state->cp++ & 0x1F);
            } else {
                /* back off to accepting the original '\' as a literal */
                --state->cp;
                c = '\\';
            }
            goto doFlat;
          /* HexEscapeSequence */
          case 'x':
            nDigits = 2;
            goto lexHex;
          /* UnicodeEscapeSequence */
          case 'u':
            nDigits = 4;
lexHex:
            n = 0;
            for (i = 0; i < nDigits && state->cp < state->cpend; i++) {
                UINT digit;
                c = *state->cp++;
                if (!isASCIIHexDigit(c, &digit)) {
                    /*
                     * Back off to accepting the original 'u' or 'x' as a
                     * literal.
                     */
                    state->cp -= i + 2;
                    n = *state->cp++;
                    break;
                }
                n = (n << 4) | digit;
            }
            c = (WCHAR) n;
            goto doFlat;
          /* Character class escapes */
          case 'd':
            state->result = NewRENode(state, REOP_DIGIT);
doSimple:
            if (!state->result)
                return FALSE;
            state->progLength++;
            break;
          case 'D':
            state->result = NewRENode(state, REOP_NONDIGIT);
            goto doSimple;
          case 's':
            state->result = NewRENode(state, REOP_SPACE);
            goto doSimple;
          case 'S':
            state->result = NewRENode(state, REOP_NONSPACE);
            goto doSimple;
          case 'w':
            state->result = NewRENode(state, REOP_ALNUM);
            goto doSimple;
          case 'W':
            state->result = NewRENode(state, REOP_NONALNUM);
            goto doSimple;
          /* IdentityEscape */
          default:
            state->result = NewRENode(state, REOP_FLAT);
            if (!state->result)
                return FALSE;
            state->result->u.flat.chr = c;
            state->result->u.flat.length = 1;
            state->result->kid = (void *) (state->cp - 1);
            state->progLength += 3;
            break;
        }
        break;
      case '[':
        state->result = NewRENode(state, REOP_CLASS);
        if (!state->result)
            return FALSE;
        termStart = state->cp;
        state->result->u.ucclass.startIndex = termStart - state->cpbegin;
        for (;;) {
            if (state->cp == state->cpend) {
                ReportRegExpErrorHelper(state, JSREPORT_ERROR,
                                        JSMSG_UNTERM_CLASS, termStart);

                return FALSE;
            }
            if (*state->cp == '\\') {
                state->cp++;
                if (state->cp != state->cpend)
                    state->cp++;
                continue;
            }
            if (*state->cp == ']') {
                state->result->u.ucclass.kidlen = state->cp - termStart;
                break;
            }
            state->cp++;
        }
        for (i = 0; i < CLASS_CACHE_SIZE; i++) {
            if (!state->classCache[i].start) {
                state->classCache[i].start = termStart;
                state->classCache[i].length = state->result->u.ucclass.kidlen;
                state->classCache[i].index = state->classCount;
                break;
            }
            if (state->classCache[i].length ==
                state->result->u.ucclass.kidlen) {
                for (n = 0; ; n++) {
                    if (n == state->classCache[i].length) {
                        state->result->u.ucclass.index
                            = state->classCache[i].index;
                        goto claim;
                    }
                    if (state->classCache[i].start[n] != termStart[n])
                        break;
                }
            }
        }
        state->result->u.ucclass.index = state->classCount++;

    claim:
        /*
         * Call CalculateBitmapSize now as we want any errors it finds
         * to be reported during the parse phase, not at execution.
         */
        if (!CalculateBitmapSize(state, state->result, termStart, state->cp++))
            return FALSE;
        /*
         * Update classBitmapsMem with number of bytes to hold bmsize bits,
         * which is (bitsCount + 7) / 8 or (highest_bit + 1 + 7) / 8
         * or highest_bit / 8 + 1 where highest_bit is u.ucclass.bmsize.
         */
        n = (state->result->u.ucclass.bmsize >> 3) + 1;
        if (n > CLASS_BITMAPS_MEM_LIMIT - state->classBitmapsMem) {
            ReportRegExpError(state, JSREPORT_ERROR, JSMSG_REGEXP_TOO_COMPLEX);
            return FALSE;
        }
        state->classBitmapsMem += n;
        /* CLASS, <index> */
        state->progLength
            += 1 + GetCompactIndexWidth(state->result->u.ucclass.index);
        break;

      case '.':
        state->result = NewRENode(state, REOP_DOT);
        goto doSimple;

      case '{':
      {
        const WCHAR *errp = state->cp--;
        INT err;

        err = ParseMinMaxQuantifier(state, TRUE);
        state->cp = errp;

        if (err < 0)
            goto asFlat;

        /* FALL THROUGH */
      }
      case '*':
      case '+':
      case '?':
        throw_error(state->context, state->context->version < SCRIPTLANGUAGEVERSION_ES5 ? JS_E_REGEXP_SYNTAX : JS_E_UNEXPECTED_QUANTIFIER, L"");
        return FALSE;
      default:
asFlat:
        state->result = NewRENode(state, REOP_FLAT);
        if (!state->result)
            return FALSE;
        state->result->u.flat.chr = c;
        state->result->u.flat.length = 1;
        state->result->kid = (void *) (state->cp - 1);
        state->progLength += 3;
        break;
    }
    return ParseQuantifier(state);
}

/*
 * Top-down regular expression grammar, based closely on Perl4.
 *
 *  regexp:     altern                  A regular expression is one or more
 *              altern '|' regexp       alternatives separated by vertical bar.
 */
#define INITIAL_STACK_SIZE  128

static BOOL
ParseRegExp(CompilerState *state)
{
    size_t parenIndex;
    RENode *operand;
    REOpData *operatorStack;
    RENode **operandStack;
    REOp op;
    INT i;
    BOOL result = FALSE;

    INT operatorSP = 0, operatorStackSize = INITIAL_STACK_SIZE;
    INT operandSP = 0, operandStackSize = INITIAL_STACK_SIZE;

    /* Watch out for empty regexp */
    if (state->cp == state->cpend) {
        state->result = NewRENode(state, REOP_EMPTY);
        return (state->result != NULL);
    }

    operatorStack = malloc(sizeof(REOpData) * operatorStackSize);
    if (!operatorStack)
        return FALSE;

    operandStack = malloc(sizeof(RENode *) * operandStackSize);
    if (!operandStack)
        goto out;

    for (;;) {
        parenIndex = state->parenCount;
        if (state->cp == state->cpend) {
            /*
             * If we are at the end of the regexp and we're short one or more
             * operands, the regexp must have the form /x|/ or some such, with
             * left parentheses making us short more than one operand.
             */
            if (operatorSP >= operandSP) {
                operand = NewRENode(state, REOP_EMPTY);
                if (!operand)
                    goto out;
                goto pushOperand;
            }
        } else {
            switch (*state->cp) {
              case '(':
                ++state->cp;
                if (state->cp + 1 < state->cpend &&
                    *state->cp == '?' &&
                    (state->cp[1] == '=' ||
                     state->cp[1] == '!' ||
                     state->cp[1] == ':')) {
                    switch (state->cp[1]) {
                      case '=':
                        op = REOP_ASSERT;
                        /* ASSERT, <next>, ... ASSERTTEST */
                        state->progLength += 4;
                        break;
                      case '!':
                        op = REOP_ASSERT_NOT;
                        /* ASSERTNOT, <next>, ... ASSERTNOTTEST */
                        state->progLength += 4;
                        break;
                      default:
                        op = REOP_LPARENNON;
                        break;
                    }
                    state->cp += 2;
                } else {
                    op = REOP_LPAREN;
                    /* LPAREN, <index>, ... RPAREN, <index> */
                    state->progLength
                        += 2 * (1 + GetCompactIndexWidth(parenIndex));
                    state->parenCount++;
                    if (state->parenCount == 65535) {
                        ReportRegExpError(state, JSREPORT_ERROR,
                                          JSMSG_TOO_MANY_PARENS);
                        goto out;
                    }
                }
                goto pushOperator;

              case ')':
                /*
                 * If there's no stacked open parenthesis, throw syntax error.
                 */
                for (i = operatorSP - 1; ; i--) {
                    if (i < 0) {
                        ReportRegExpError(state, JSREPORT_ERROR,
                                          JSMSG_UNMATCHED_RIGHT_PAREN);
                        goto out;
                    }
                    if (operatorStack[i].op == REOP_ASSERT ||
                        operatorStack[i].op == REOP_ASSERT_NOT ||
                        operatorStack[i].op == REOP_LPARENNON ||
                        operatorStack[i].op == REOP_LPAREN) {
                        break;
                    }
                }
                /* FALL THROUGH */

              case '|':
                /* Expected an operand before these, so make an empty one */
                operand = NewRENode(state, REOP_EMPTY);
                if (!operand)
                    goto out;
                goto pushOperand;

              default:
                if (!ParseTerm(state))
                    goto out;
                operand = state->result;
pushOperand:
                if (operandSP == operandStackSize) {
                    RENode **tmp;
                    operandStackSize += operandStackSize;
                    tmp = realloc(operandStack, sizeof(RENode *) * operandStackSize);
                    if (!tmp)
                        goto out;
                    operandStack = tmp;
                }
                operandStack[operandSP++] = operand;
                break;
            }
        }

        /* At the end; process remaining operators. */
restartOperator:
        if (state->cp == state->cpend) {
            while (operatorSP) {
                --operatorSP;
                if (!ProcessOp(state, &operatorStack[operatorSP],
                               operandStack, operandSP))
                    goto out;
                --operandSP;
            }
            assert(operandSP == 1);
            state->result = operandStack[0];
            result = TRUE;
            goto out;
        }

        switch (*state->cp) {
          case '|':
            /* Process any stacked 'concat' operators */
            ++state->cp;
            while (operatorSP &&
                   operatorStack[operatorSP - 1].op == REOP_CONCAT) {
                --operatorSP;
                if (!ProcessOp(state, &operatorStack[operatorSP],
                               operandStack, operandSP)) {
                    goto out;
                }
                --operandSP;
            }
            op = REOP_ALT;
            goto pushOperator;

          case ')':
            /*
             * If there's no stacked open parenthesis, throw syntax error.
             */
            for (i = operatorSP - 1; ; i--) {
                if (i < 0) {
                    ReportRegExpError(state, JSREPORT_ERROR,
                                      JSMSG_UNMATCHED_RIGHT_PAREN);
                    goto out;
                }
                if (operatorStack[i].op == REOP_ASSERT ||
                    operatorStack[i].op == REOP_ASSERT_NOT ||
                    operatorStack[i].op == REOP_LPARENNON ||
                    operatorStack[i].op == REOP_LPAREN) {
                    break;
                }
            }
            ++state->cp;

            /* Process everything on the stack until the open parenthesis. */
            for (;;) {
                assert(operatorSP);
                --operatorSP;
                switch (operatorStack[operatorSP].op) {
                  case REOP_ASSERT:
                  case REOP_ASSERT_NOT:
                  case REOP_LPAREN:
                    operand = NewRENode(state, operatorStack[operatorSP].op);
                    if (!operand)
                        goto out;
                    operand->u.parenIndex =
                        operatorStack[operatorSP].parenIndex;
                    assert(operandSP);
                    operand->kid = operandStack[operandSP - 1];
                    operandStack[operandSP - 1] = operand;
                    if (state->treeDepth == TREE_DEPTH_MAX) {
                        ReportRegExpError(state, JSREPORT_ERROR,
                                          JSMSG_REGEXP_TOO_COMPLEX);
                        goto out;
                    }
                    ++state->treeDepth;
                    /* FALL THROUGH */

                  case REOP_LPARENNON:
                    state->result = operandStack[operandSP - 1];
                    if (!ParseQuantifier(state))
                        goto out;
                    operandStack[operandSP - 1] = state->result;
                    goto restartOperator;
                  default:
                    if (!ProcessOp(state, &operatorStack[operatorSP],
                                   operandStack, operandSP))
                        goto out;
                    --operandSP;
                    break;
                }
            }
            break;

          case '{':
          {
            const WCHAR *errp = state->cp;

            if (ParseMinMaxQuantifier(state, TRUE) < 0) {
                /*
                 * This didn't even scan correctly as a quantifier, so we should
                 * treat it as flat.
                 */
                op = REOP_CONCAT;
                goto pushOperator;
            }

            state->cp = errp;
            /* FALL THROUGH */
          }

          case '+':
          case '*':
          case '?':
            ReportRegExpErrorHelper(state, JSREPORT_ERROR, JSMSG_BAD_QUANTIFIER,
                                    state->cp);
            result = FALSE;
            goto out;

          default:
            /* Anything else is the start of the next term. */
            op = REOP_CONCAT;
pushOperator:
            if (operatorSP == operatorStackSize) {
                REOpData *tmp;
                operatorStackSize += operatorStackSize;
                tmp = realloc(operatorStack, sizeof(REOpData) * operatorStackSize);
                if (!tmp)
                    goto out;
                operatorStack = tmp;
            }
            operatorStack[operatorSP].op = op;
            operatorStack[operatorSP].errPos = state->cp;
            operatorStack[operatorSP++].parenIndex = parenIndex;
            break;
        }
    }
out:
    free(operatorStack);
    free(operandStack);
    return result;
}

/*
 * Save the current state of the match - the position in the input
 * text as well as the position in the bytecode. The state of any
 * parent expressions is also saved (preceding state).
 * Contents of parenCount parentheses from parenIndex are also saved.
 */
static REBackTrackData *
PushBackTrackState(REGlobalData *gData, REOp op,
                   jsbytecode *target, match_state_t *x, const WCHAR *cp,
                   size_t parenIndex, size_t parenCount)
{
    size_t i;
    REBackTrackData *result =
        (REBackTrackData *) ((char *)gData->backTrackSP + gData->cursz);

    size_t sz = sizeof(REBackTrackData) +
                gData->stateStackTop * sizeof(REProgState) +
                parenCount * sizeof(RECapture);

    ptrdiff_t btsize = gData->backTrackStackSize;
    ptrdiff_t btincr = ((char *)result + sz) -
                       ((char *)gData->backTrackStack + btsize);

    TRACE("\tBT_Push: %Iu,%Iu\n", (ULONG_PTR)parenIndex, (ULONG_PTR)parenCount);

    JS_COUNT_OPERATION(gData->cx, JSOW_JUMP * (1 + parenCount));
    if (btincr > 0) {
        ptrdiff_t offset = (char *)result - (char *)gData->backTrackStack;

        JS_COUNT_OPERATION(gData->cx, JSOW_ALLOCATION);
        btincr = ((btincr+btsize-1)/btsize)*btsize;
        gData->backTrackStack = heap_pool_grow(gData->pool, gData->backTrackStack, btsize, btincr);
        if (!gData->backTrackStack) {
            throw_error(gData->cx, E_OUTOFMEMORY, L"");
            gData->ok = FALSE;
            return NULL;
        }
        gData->backTrackStackSize = btsize + btincr;
        result = (REBackTrackData *) ((char *)gData->backTrackStack + offset);
    }
    gData->backTrackSP = result;
    result->sz = gData->cursz;
    gData->cursz = sz;

    result->backtrack_op = op;
    result->backtrack_pc = target;
    result->cp = cp;
    result->parenCount = parenCount;
    result->parenIndex = parenIndex;

    result->saveStateStackTop = gData->stateStackTop;
    assert(gData->stateStackTop);
    memcpy(result + 1, gData->stateStack,
           sizeof(REProgState) * result->saveStateStackTop);

    if (parenCount != 0) {
        memcpy((char *)(result + 1) +
               sizeof(REProgState) * result->saveStateStackTop,
               &x->parens[parenIndex],
               sizeof(RECapture) * parenCount);
        for (i = 0; i != parenCount; i++)
            x->parens[parenIndex + i].index = -1;
    }

    return result;
}

static inline match_state_t *
FlatNIMatcher(REGlobalData *gData, match_state_t *x, const WCHAR *matchChars,
              size_t length)
{
    size_t i;
    assert(gData->cpend >= x->cp);
    if (length > (size_t)(gData->cpend - x->cp))
        return NULL;
    for (i = 0; i != length; i++) {
        if (towupper(matchChars[i]) != towupper(x->cp[i]))
            return NULL;
    }
    x->cp += length;
    return x;
}

/*
 * 1. Evaluate DecimalEscape to obtain an EscapeValue E.
 * 2. If E is not a character then go to step 6.
 * 3. Let ch be E's character.
 * 4. Let A be a one-element RECharSet containing the character ch.
 * 5. Call CharacterSetMatcher(A, false) and return its Matcher result.
 * 6. E must be an integer. Let n be that integer.
 * 7. If n=0 or n>NCapturingParens then throw a SyntaxError exception.
 * 8. Return an internal Matcher closure that takes two arguments, a State x
 *    and a Continuation c, and performs the following:
 *     1. Let cap be x's captures internal array.
 *     2. Let s be cap[n].
 *     3. If s is undefined, then call c(x) and return its result.
 *     4. Let e be x's endIndex.
 *     5. Let len be s's length.
 *     6. Let f be e+len.
 *     7. If f>InputLength, return failure.
 *     8. If there exists an integer i between 0 (inclusive) and len (exclusive)
 *        such that Canonicalize(s[i]) is not the same character as
 *        Canonicalize(Input [e+i]), then return failure.
 *     9. Let y be the State (f, cap).
 *     10. Call c(y) and return its result.
 */
static match_state_t *
BackrefMatcher(REGlobalData *gData, match_state_t *x, size_t parenIndex)
{
    size_t len, i;
    const WCHAR *parenContent;
    RECapture *cap = &x->parens[parenIndex];

    if (cap->index == -1)
        return x;

    len = cap->length;
    if (x->cp + len > gData->cpend)
        return NULL;

    parenContent = &gData->cpbegin[cap->index];
    if (gData->regexp->flags & REG_FOLD) {
        for (i = 0; i < len; i++) {
            if (towupper(parenContent[i]) != towupper(x->cp[i]))
                return NULL;
        }
    } else {
        for (i = 0; i < len; i++) {
            if (parenContent[i] != x->cp[i])
                return NULL;
        }
    }
    x->cp += len;
    return x;
}

/* Add a single character to the RECharSet */
static void
AddCharacterToCharSet(RECharSet *cs, WCHAR c)
{
    UINT byteIndex = (UINT)(c >> 3);
    assert(c <= cs->length);
    cs->u.bits[byteIndex] |= 1 << (c & 0x7);
}


/* Add a character range, c1 to c2 (inclusive) to the RECharSet */
static void
AddCharacterRangeToCharSet(RECharSet *cs, UINT c1, UINT c2)
{
    UINT i;

    UINT byteIndex1 = c1 >> 3;
    UINT byteIndex2 = c2 >> 3;

    assert(c2 <= cs->length && c1 <= c2);

    c1 &= 0x7;
    c2 &= 0x7;

    if (byteIndex1 == byteIndex2) {
        cs->u.bits[byteIndex1] |= ((BYTE)0xFF >> (7 - (c2 - c1))) << c1;
    } else {
        cs->u.bits[byteIndex1] |= 0xFF << c1;
        for (i = byteIndex1 + 1; i < byteIndex2; i++)
            cs->u.bits[i] = 0xFF;
        cs->u.bits[byteIndex2] |= (BYTE)0xFF >> (7 - c2);
    }
}

/* Compile the source of the class into a RECharSet */
static BOOL
ProcessCharSet(REGlobalData *gData, RECharSet *charSet)
{
    const WCHAR *src, *end;
    BOOL inRange = FALSE;
    WCHAR rangeStart = 0;
    UINT byteLength, n;
    WCHAR c, thisCh;
    INT nDigits, i;

    assert(!charSet->converted);
    /*
     * Assert that startIndex and length points to chars inside [] inside
     * source string.
     */
    assert(1 <= charSet->u.src.startIndex);
    assert(charSet->u.src.startIndex < gData->regexp->source_len);
    assert(charSet->u.src.length <= gData->regexp->source_len
            - 1 - charSet->u.src.startIndex);

    charSet->converted = TRUE;
    src = gData->regexp->source + charSet->u.src.startIndex;

    end = src + charSet->u.src.length;

    assert(src[-1] == '[' && end[0] == ']');

    byteLength = (charSet->length >> 3) + 1;
    charSet->u.bits = malloc(byteLength);
    if (!charSet->u.bits) {
        throw_error(gData->cx, E_OUTOFMEMORY, L"");
        gData->ok = FALSE;
        return FALSE;
    }
    memset(charSet->u.bits, 0, byteLength);

    if (src == end)
        return TRUE;

    if (*src == '^') {
        assert(charSet->sense == FALSE);
        ++src;
    } else {
        assert(charSet->sense == TRUE);
    }

    while (src != end) {
        switch (*src) {
          case '\\':
            ++src;
            c = *src++;
            switch (c) {
              case 'b':
                thisCh = 0x8;
                break;
              case 'f':
                thisCh = 0xC;
                break;
              case 'n':
                thisCh = 0xA;
                break;
              case 'r':
                thisCh = 0xD;
                break;
              case 't':
                thisCh = 0x9;
                break;
              case 'v':
                thisCh = 0xB;
                break;
              case 'c':
                if (src < end && JS_ISWORD(*src)) {
                    thisCh = (WCHAR)(*src++ & 0x1F);
                } else {
                    --src;
                    thisCh = '\\';
                }
                break;
              case 'x':
                nDigits = 2;
                goto lexHex;
              case 'u':
                nDigits = 4;
            lexHex:
                n = 0;
                for (i = 0; (i < nDigits) && (src < end); i++) {
                    UINT digit;
                    c = *src++;
                    if (!isASCIIHexDigit(c, &digit)) {
                        /*
                         * Back off to accepting the original '\'
                         * as a literal
                         */
                        src -= i + 1;
                        n = '\\';
                        break;
                    }
                    n = (n << 4) | digit;
                }
                thisCh = (WCHAR)n;
                break;
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
                /*
                 *  This is a non-ECMA extension - decimal escapes (in this
                 *  case, octal!) are supposed to be an error inside class
                 *  ranges, but supported here for backwards compatibility.
                 */
                n = JS7_UNDEC(c);
                c = *src;
                if ('0' <= c && c <= '7') {
                    src++;
                    n = 8 * n + JS7_UNDEC(c);
                    c = *src;
                    if ('0' <= c && c <= '7') {
                        src++;
                        i = 8 * n + JS7_UNDEC(c);
                        if (i <= 0377)
                            n = i;
                        else
                            src--;
                    }
                }
                thisCh = (WCHAR)n;
                break;

              case 'd':
                AddCharacterRangeToCharSet(charSet, '0', '9');
                continue;   /* don't need range processing */
              case 'D':
                AddCharacterRangeToCharSet(charSet, 0, '0' - 1);
                AddCharacterRangeToCharSet(charSet,
                                           (WCHAR)('9' + 1),
                                           (WCHAR)charSet->length);
                continue;
              case 's':
                for (i = (INT)charSet->length; i >= 0; i--)
                    if (iswspace(i))
                        AddCharacterToCharSet(charSet, (WCHAR)i);
                continue;
              case 'S':
                for (i = (INT)charSet->length; i >= 0; i--)
                    if (!iswspace(i))
                        AddCharacterToCharSet(charSet, (WCHAR)i);
                continue;
              case 'w':
                for (i = (INT)charSet->length; i >= 0; i--)
                    if (JS_ISWORD(i))
                        AddCharacterToCharSet(charSet, (WCHAR)i);
                continue;
              case 'W':
                for (i = (INT)charSet->length; i >= 0; i--)
                    if (!JS_ISWORD(i))
                        AddCharacterToCharSet(charSet, (WCHAR)i);
                continue;
              default:
                thisCh = c;
                break;

            }
            break;

          default:
            thisCh = *src++;
            break;

        }
        if (inRange) {
            if (gData->regexp->flags & REG_FOLD) {
                assert(rangeStart <= thisCh);
                for (i = rangeStart; i <= thisCh; i++) {
                    WCHAR uch, dch;

                    AddCharacterToCharSet(charSet, i);
                    uch = towupper(i);
                    dch = towlower(i);
                    if (i != uch)
                        AddCharacterToCharSet(charSet, uch);
                    if (i != dch)
                        AddCharacterToCharSet(charSet, dch);
                }
            } else {
                AddCharacterRangeToCharSet(charSet, rangeStart, thisCh);
            }
            inRange = FALSE;
        } else {
            if (gData->regexp->flags & REG_FOLD) {
                AddCharacterToCharSet(charSet, towupper(thisCh));
                AddCharacterToCharSet(charSet, towlower(thisCh));
            } else {
                AddCharacterToCharSet(charSet, thisCh);
            }
            if (src < end - 1) {
                if (*src == '-') {
                    ++src;
                    inRange = TRUE;
                    rangeStart = thisCh;
                }
            }
        }
    }
    return TRUE;
}

static BOOL
ReallocStateStack(REGlobalData *gData)
{
    size_t limit = gData->stateStackLimit;
    size_t sz = sizeof(REProgState) * limit;

    gData->stateStack = heap_pool_grow(gData->pool, gData->stateStack, sz, sz);
    if (!gData->stateStack) {
        throw_error(gData->cx, E_OUTOFMEMORY, L"");
        gData->ok = FALSE;
        return FALSE;
    }
    gData->stateStackLimit = limit + limit;
    return TRUE;
}

#define PUSH_STATE_STACK(data)                                                \
    do {                                                                      \
        ++(data)->stateStackTop;                                              \
        if ((data)->stateStackTop == (data)->stateStackLimit &&               \
            !ReallocStateStack((data))) {                                     \
            return NULL;                                                      \
        }                                                                     \
    }while(0)

/*
 * Apply the current op against the given input to see if it's going to match
 * or fail. Return false if we don't get a match, true if we do. If updatecp is
 * true, then update the current state's cp. Always update startpc to the next
 * op.
 */
static inline match_state_t *
SimpleMatch(REGlobalData *gData, match_state_t *x, REOp op,
            jsbytecode **startpc, BOOL updatecp)
{
    match_state_t *result = NULL;
    WCHAR matchCh;
    size_t parenIndex;
    size_t offset, length, index;
    jsbytecode *pc = *startpc;  /* pc has already been incremented past op */
    const WCHAR *source;
    const WCHAR *startcp = x->cp;
    WCHAR ch;
    RECharSet *charSet;

    const char *opname = reop_names[op];
    TRACE("\n%06d: %*s%s\n", (int)(pc - gData->regexp->program),
          (int)gData->stateStackTop * 2, "", opname);

    switch (op) {
      case REOP_EMPTY:
        result = x;
        break;
      case REOP_BOL:
        if (x->cp != gData->cpbegin) {
            if (/*!gData->cx->regExpStatics.multiline &&  FIXME !!! */
                !(gData->regexp->flags & REG_MULTILINE)) {
                break;
            }
            if (!RE_IS_LINE_TERM(x->cp[-1]))
                break;
        }
        result = x;
        break;
      case REOP_EOL:
        if (x->cp != gData->cpend) {
            if (/*!gData->cx->regExpStatics.multiline &&*/
                !(gData->regexp->flags & REG_MULTILINE)) {
                break;
            }
            if (!RE_IS_LINE_TERM(*x->cp))
                break;
        }
        result = x;
        break;
      case REOP_WBDRY:
        if ((x->cp == gData->cpbegin || !JS_ISWORD(x->cp[-1])) ^
            !(x->cp != gData->cpend && JS_ISWORD(*x->cp))) {
            result = x;
        }
        break;
      case REOP_WNONBDRY:
        if ((x->cp == gData->cpbegin || !JS_ISWORD(x->cp[-1])) ^
            (x->cp != gData->cpend && JS_ISWORD(*x->cp))) {
            result = x;
        }
        break;
      case REOP_DOT:
        if (x->cp != gData->cpend && !RE_IS_LINE_TERM(*x->cp)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_DIGIT:
        if (x->cp != gData->cpend && JS7_ISDEC(*x->cp)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_NONDIGIT:
        if (x->cp != gData->cpend && !JS7_ISDEC(*x->cp)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_ALNUM:
        if (x->cp != gData->cpend && JS_ISWORD(*x->cp)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_NONALNUM:
        if (x->cp != gData->cpend && !JS_ISWORD(*x->cp)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_SPACE:
        if (x->cp != gData->cpend && iswspace(*x->cp)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_NONSPACE:
        if (x->cp != gData->cpend && !iswspace(*x->cp)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_BACKREF:
        pc = ReadCompactIndex(pc, &parenIndex);
        assert(parenIndex < gData->regexp->parenCount);
        result = BackrefMatcher(gData, x, parenIndex);
        break;
      case REOP_FLAT:
        pc = ReadCompactIndex(pc, &offset);
        assert(offset < gData->regexp->source_len);
        pc = ReadCompactIndex(pc, &length);
        assert(1 <= length);
        assert(length <= gData->regexp->source_len - offset);
        if (length <= (size_t)(gData->cpend - x->cp)) {
            source = gData->regexp->source + offset;
            TRACE("%s\n", debugstr_wn(source, length));
            for (index = 0; index != length; index++) {
                if (source[index] != x->cp[index])
                    return NULL;
            }
            x->cp += length;
            result = x;
        }
        break;
      case REOP_FLAT1:
        matchCh = *pc++;
        TRACE(" '%c' == '%c'\n", (char)matchCh, (char)*x->cp);
        if (x->cp != gData->cpend && *x->cp == matchCh) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_FLATi:
        pc = ReadCompactIndex(pc, &offset);
        assert(offset < gData->regexp->source_len);
        pc = ReadCompactIndex(pc, &length);
        assert(1 <= length);
        assert(length <= gData->regexp->source_len - offset);
        source = gData->regexp->source;
        result = FlatNIMatcher(gData, x, source + offset, length);
        break;
      case REOP_FLAT1i:
        matchCh = *pc++;
        if (x->cp != gData->cpend && towupper(*x->cp) == towupper(matchCh)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_UCFLAT1:
        matchCh = GET_ARG(pc);
        TRACE(" '%c' == '%c'\n", (char)matchCh, (char)*x->cp);
        pc += ARG_LEN;
        if (x->cp != gData->cpend && *x->cp == matchCh) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_UCFLAT1i:
        matchCh = GET_ARG(pc);
        pc += ARG_LEN;
        if (x->cp != gData->cpend && towupper(*x->cp) == towupper(matchCh)) {
            result = x;
            result->cp++;
        }
        break;
      case REOP_CLASS:
        pc = ReadCompactIndex(pc, &index);
        assert(index < gData->regexp->classCount);
        if (x->cp != gData->cpend) {
            charSet = &gData->regexp->classList[index];
            assert(charSet->converted);
            ch = *x->cp;
            index = ch >> 3;
            if (charSet->length != 0 &&
                ch <= charSet->length &&
                (charSet->u.bits[index] & (1 << (ch & 0x7)))) {
                result = x;
                result->cp++;
            }
        }
        break;
      case REOP_NCLASS:
        pc = ReadCompactIndex(pc, &index);
        assert(index < gData->regexp->classCount);
        if (x->cp != gData->cpend) {
            charSet = &gData->regexp->classList[index];
            assert(charSet->converted);
            ch = *x->cp;
            index = ch >> 3;
            if (charSet->length == 0 ||
                ch > charSet->length ||
                !(charSet->u.bits[index] & (1 << (ch & 0x7)))) {
                result = x;
                result->cp++;
            }
        }
        break;

      default:
        assert(FALSE);
    }
    if (result) {
        if (!updatecp)
            x->cp = startcp;
        *startpc = pc;
        TRACE(" *\n");
        return result;
    }
    x->cp = startcp;
    return NULL;
}

static inline match_state_t *
ExecuteREBytecode(REGlobalData *gData, match_state_t *x)
{
    match_state_t *result = NULL;
    REBackTrackData *backTrackData;
    jsbytecode *nextpc, *testpc;
    REOp nextop;
    RECapture *cap;
    REProgState *curState;
    const WCHAR *startcp;
    size_t parenIndex, k;
    size_t parenSoFar = 0;

    WCHAR matchCh1, matchCh2;
    RECharSet *charSet;

    BOOL anchor;
    jsbytecode *pc = gData->regexp->program;
    REOp op = (REOp) *pc++;

    /*
     * If the first node is a simple match, step the index into the string
     * until that match is made, or fail if it can't be found at all.
     */
    if (REOP_IS_SIMPLE(op) && !(gData->regexp->flags & REG_STICKY)) {
        anchor = FALSE;
        while (x->cp <= gData->cpend) {
            nextpc = pc;    /* reset back to start each time */
            result = SimpleMatch(gData, x, op, &nextpc, TRUE);
            if (result) {
                anchor = TRUE;
                x = result;
                pc = nextpc;    /* accept skip to next opcode */
                op = (REOp) *pc++;
                assert(op < REOP_LIMIT);
                break;
            }
            gData->skipped++;
            x->cp++;
        }
        if (!anchor)
            goto bad;
    }

    for (;;) {
        const char *opname = reop_names[op];
        TRACE("\n%06d: %*s%s\n", (int)(pc - gData->regexp->program),
              (int)gData->stateStackTop * 2, "", opname);

        if (REOP_IS_SIMPLE(op)) {
            result = SimpleMatch(gData, x, op, &pc, TRUE);
        } else {
            curState = &gData->stateStack[gData->stateStackTop];
            switch (op) {
              case REOP_END:
                goto good;
              case REOP_ALTPREREQ2:
                nextpc = pc + GET_OFFSET(pc);   /* start of next op */
                pc += ARG_LEN;
                matchCh2 = GET_ARG(pc);
                pc += ARG_LEN;
                k = GET_ARG(pc);
                pc += ARG_LEN;

                if (x->cp != gData->cpend) {
                    if (*x->cp == matchCh2)
                        goto doAlt;

                    charSet = &gData->regexp->classList[k];
                    if (!charSet->converted && !ProcessCharSet(gData, charSet))
                        goto bad;
                    matchCh1 = *x->cp;
                    k = matchCh1 >> 3;
                    if ((charSet->length == 0 ||
                         matchCh1 > charSet->length ||
                         !(charSet->u.bits[k] & (1 << (matchCh1 & 0x7)))) ^
                        charSet->sense) {
                        goto doAlt;
                    }
                }
                result = NULL;
                break;

              case REOP_ALTPREREQ:
                nextpc = pc + GET_OFFSET(pc);   /* start of next op */
                pc += ARG_LEN;
                matchCh1 = GET_ARG(pc);
                pc += ARG_LEN;
                matchCh2 = GET_ARG(pc);
                pc += ARG_LEN;
                if (x->cp == gData->cpend ||
                    (*x->cp != matchCh1 && *x->cp != matchCh2)) {
                    result = NULL;
                    break;
                }
                /* else fall through... */

              case REOP_ALT:
              doAlt:
                nextpc = pc + GET_OFFSET(pc);   /* start of next alternate */
                pc += ARG_LEN;                  /* start of this alternate */
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                op = (REOp) *pc++;
                startcp = x->cp;
                if (REOP_IS_SIMPLE(op)) {
                    if (!SimpleMatch(gData, x, op, &pc, TRUE)) {
                        op = (REOp) *nextpc++;
                        pc = nextpc;
                        continue;
                    }
                    result = x;
                    op = (REOp) *pc++;
                }
                nextop = (REOp) *nextpc++;
                if (!PushBackTrackState(gData, nextop, nextpc, x, startcp, 0, 0))
                    goto bad;
                continue;

              /*
               * Occurs at (successful) end of REOP_ALT,
               */
              case REOP_JUMP:
                /*
                 * If we have not gotten a result here, it is because of an
                 * empty match.  Do the same thing REOP_EMPTY would do.
                 */
                if (!result)
                    result = x;

                --gData->stateStackTop;
                pc += GET_OFFSET(pc);
                op = (REOp) *pc++;
                continue;

              /*
               * Occurs at last (successful) end of REOP_ALT,
               */
              case REOP_ENDALT:
                /*
                 * If we have not gotten a result here, it is because of an
                 * empty match.  Do the same thing REOP_EMPTY would do.
                 */
                if (!result)
                    result = x;

                --gData->stateStackTop;
                op = (REOp) *pc++;
                continue;

              case REOP_LPAREN:
                pc = ReadCompactIndex(pc, &parenIndex);
                TRACE("[ %Iu ]\n", (ULONG_PTR)parenIndex);
                assert(parenIndex < gData->regexp->parenCount);
                if (parenIndex + 1 > parenSoFar)
                    parenSoFar = parenIndex + 1;
                x->parens[parenIndex].index = x->cp - gData->cpbegin;
                x->parens[parenIndex].length = 0;
                op = (REOp) *pc++;
                continue;

              case REOP_RPAREN:
              {
                ptrdiff_t delta;

                pc = ReadCompactIndex(pc, &parenIndex);
                assert(parenIndex < gData->regexp->parenCount);
                cap = &x->parens[parenIndex];
                delta = x->cp - (gData->cpbegin + cap->index);
                cap->length = (delta < 0) ? 0 : (size_t) delta;
                op = (REOp) *pc++;

                if (!result)
                    result = x;
                continue;
              }
              case REOP_ASSERT:
                nextpc = pc + GET_OFFSET(pc);  /* start of term after ASSERT */
                pc += ARG_LEN;                 /* start of ASSERT child */
                op = (REOp) *pc++;
                testpc = pc;
                if (REOP_IS_SIMPLE(op) &&
                    !SimpleMatch(gData, x, op, &testpc, FALSE)) {
                    result = NULL;
                    break;
                }
                curState->u.assertion.top =
                    (char *)gData->backTrackSP - (char *)gData->backTrackStack;
                curState->u.assertion.sz = gData->cursz;
                curState->index = x->cp - gData->cpbegin;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (!PushBackTrackState(gData, REOP_ASSERTTEST,
                                        nextpc, x, x->cp, 0, 0)) {
                    goto bad;
                }
                continue;

              case REOP_ASSERT_NOT:
                nextpc = pc + GET_OFFSET(pc);
                pc += ARG_LEN;
                op = (REOp) *pc++;
                testpc = pc;
                if (REOP_IS_SIMPLE(op) /* Note - fail to fail! */ &&
                    SimpleMatch(gData, x, op, &testpc, FALSE) &&
                    *testpc == REOP_ASSERTNOTTEST) {
                    result = NULL;
                    break;
                }
                curState->u.assertion.top
                    = (char *)gData->backTrackSP -
                      (char *)gData->backTrackStack;
                curState->u.assertion.sz = gData->cursz;
                curState->index = x->cp - gData->cpbegin;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (!PushBackTrackState(gData, REOP_ASSERTNOTTEST,
                                        nextpc, x, x->cp, 0, 0)) {
                    goto bad;
                }
                continue;

              case REOP_ASSERTTEST:
                --gData->stateStackTop;
                --curState;
                x->cp = gData->cpbegin + curState->index;
                gData->backTrackSP =
                    (REBackTrackData *) ((char *)gData->backTrackStack +
                                         curState->u.assertion.top);
                gData->cursz = curState->u.assertion.sz;
                if (result)
                    result = x;
                break;

              case REOP_ASSERTNOTTEST:
                --gData->stateStackTop;
                --curState;
                x->cp = gData->cpbegin + curState->index;
                gData->backTrackSP =
                    (REBackTrackData *) ((char *)gData->backTrackStack +
                                         curState->u.assertion.top);
                gData->cursz = curState->u.assertion.sz;
                result = (!result) ? x : NULL;
                break;
              case REOP_STAR:
                curState->u.quantifier.min = 0;
                curState->u.quantifier.max = (UINT)-1;
                goto quantcommon;
              case REOP_PLUS:
                curState->u.quantifier.min = 1;
                curState->u.quantifier.max = (UINT)-1;
                goto quantcommon;
              case REOP_OPT:
                curState->u.quantifier.min = 0;
                curState->u.quantifier.max = 1;
                goto quantcommon;
              case REOP_QUANT:
                pc = ReadCompactIndex(pc, &k);
                curState->u.quantifier.min = k;
                pc = ReadCompactIndex(pc, &k);
                /* max is k - 1 to use one byte for (UINT)-1 sentinel. */
                curState->u.quantifier.max = k - 1;
                assert(curState->u.quantifier.min <= curState->u.quantifier.max);
              quantcommon:
                if (curState->u.quantifier.max == 0) {
                    pc = pc + GET_OFFSET(pc);
                    op = (REOp) *pc++;
                    result = x;
                    continue;
                }
                /* Step over <next> */
                nextpc = pc + ARG_LEN;
                op = (REOp) *nextpc++;
                startcp = x->cp;
                if (REOP_IS_SIMPLE(op)) {
                    if (!SimpleMatch(gData, x, op, &nextpc, TRUE)) {
                        if (curState->u.quantifier.min == 0)
                            result = x;
                        else
                            result = NULL;
                        pc = pc + GET_OFFSET(pc);
                        break;
                    }
                    op = (REOp) *nextpc++;
                    result = x;
                }
                curState->index = startcp - gData->cpbegin;
                curState->continue_op = REOP_REPEAT;
                curState->continue_pc = pc;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (curState->u.quantifier.min == 0 &&
                    !PushBackTrackState(gData, REOP_REPEAT, pc, x, startcp,
                                        0, 0)) {
                    goto bad;
                }
                pc = nextpc;
                continue;

              case REOP_ENDCHILD: /* marks the end of a quantifier child */
                pc = curState[-1].continue_pc;
                op = (REOp) curState[-1].continue_op;

                if (!result)
                    result = x;
                continue;

              case REOP_REPEAT:
                --curState;
                do {
                    --gData->stateStackTop;
                    if (!result) {
                        /* Failed, see if we have enough children. */
                        if (curState->u.quantifier.min == 0)
                            goto repeatDone;
                        goto break_switch;
                    }
                    if (curState->u.quantifier.min == 0 &&
                        x->cp == gData->cpbegin + curState->index) {
                        /* matched an empty string, that'll get us nowhere */
                        result = NULL;
                        goto break_switch;
                    }
                    if (curState->u.quantifier.min != 0)
                        curState->u.quantifier.min--;
                    if (curState->u.quantifier.max != (UINT) -1)
                        curState->u.quantifier.max--;
                    if (curState->u.quantifier.max == 0)
                        goto repeatDone;
                    nextpc = pc + ARG_LEN;
                    nextop = (REOp) *nextpc;
                    startcp = x->cp;
                    if (REOP_IS_SIMPLE(nextop)) {
                        nextpc++;
                        if (!SimpleMatch(gData, x, nextop, &nextpc, TRUE)) {
                            if (curState->u.quantifier.min == 0)
                                goto repeatDone;
                            result = NULL;
                            goto break_switch;
                        }
                        result = x;
                    }
                    curState->index = startcp - gData->cpbegin;
                    PUSH_STATE_STACK(gData);
                    if (curState->u.quantifier.min == 0 &&
                        !PushBackTrackState(gData, REOP_REPEAT,
                                            pc, x, startcp,
                                            curState->parenSoFar,
                                            parenSoFar -
                                            curState->parenSoFar)) {
                        goto bad;
                    }
                } while (*nextpc == REOP_ENDCHILD);
                pc = nextpc;
                op = (REOp) *pc++;
                parenSoFar = curState->parenSoFar;
                continue;

              repeatDone:
                result = x;
                pc += GET_OFFSET(pc);
                goto break_switch;

              case REOP_MINIMALSTAR:
                curState->u.quantifier.min = 0;
                curState->u.quantifier.max = (UINT)-1;
                goto minimalquantcommon;
              case REOP_MINIMALPLUS:
                curState->u.quantifier.min = 1;
                curState->u.quantifier.max = (UINT)-1;
                goto minimalquantcommon;
              case REOP_MINIMALOPT:
                curState->u.quantifier.min = 0;
                curState->u.quantifier.max = 1;
                goto minimalquantcommon;
              case REOP_MINIMALQUANT:
                pc = ReadCompactIndex(pc, &k);
                curState->u.quantifier.min = k;
                pc = ReadCompactIndex(pc, &k);
                /* See REOP_QUANT comments about k - 1. */
                curState->u.quantifier.max = k - 1;
                assert(curState->u.quantifier.min
                          <= curState->u.quantifier.max);
              minimalquantcommon:
                curState->index = x->cp - gData->cpbegin;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (curState->u.quantifier.min != 0) {
                    curState->continue_op = REOP_MINIMALREPEAT;
                    curState->continue_pc = pc;
                    /* step over <next> */
                    pc += OFFSET_LEN;
                    op = (REOp) *pc++;
                } else {
                    if (!PushBackTrackState(gData, REOP_MINIMALREPEAT,
                                            pc, x, x->cp, 0, 0)) {
                        goto bad;
                    }
                    --gData->stateStackTop;
                    pc = pc + GET_OFFSET(pc);
                    op = (REOp) *pc++;
                }
                continue;

              case REOP_MINIMALREPEAT:
                --gData->stateStackTop;
                --curState;

                TRACE("{%d,%d}\n", curState->u.quantifier.min, curState->u.quantifier.max);
#define PREPARE_REPEAT()                                                      \
    do {                                                                      \
        curState->index = x->cp - gData->cpbegin;                          \
        curState->continue_op = REOP_MINIMALREPEAT;                           \
        curState->continue_pc = pc;                                           \
        pc += ARG_LEN;                                                        \
        for (k = curState->parenSoFar; k < parenSoFar; k++)                   \
            x->parens[k].index = -1;                                          \
        PUSH_STATE_STACK(gData);                                              \
        op = (REOp) *pc++;                                                    \
        assert(op < REOP_LIMIT);                                              \
    }while(0)

                if (!result) {
                    TRACE(" -\n");
                    /*
                     * Non-greedy failure - try to consume another child.
                     */
                    if (curState->u.quantifier.max == (UINT) -1 ||
                        curState->u.quantifier.max > 0) {
                        PREPARE_REPEAT();
                        continue;
                    }
                    /* Don't need to adjust pc since we're going to pop. */
                    break;
                }
                if (curState->u.quantifier.min == 0 &&
                    x->cp == gData->cpbegin + curState->index) {
                    /* Matched an empty string, that'll get us nowhere. */
                    result = NULL;
                    break;
                }
                if (curState->u.quantifier.min != 0)
                    curState->u.quantifier.min--;
                if (curState->u.quantifier.max != (UINT) -1)
                    curState->u.quantifier.max--;
                if (curState->u.quantifier.min != 0) {
                    PREPARE_REPEAT();
                    continue;
                }
                curState->index = x->cp - gData->cpbegin;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (!PushBackTrackState(gData, REOP_MINIMALREPEAT,
                                        pc, x, x->cp,
                                        curState->parenSoFar,
                                        parenSoFar - curState->parenSoFar)) {
                    goto bad;
                }
                --gData->stateStackTop;
                pc = pc + GET_OFFSET(pc);
                op = (REOp) *pc++;
                assert(op < REOP_LIMIT);
                continue;
              default:
                assert(FALSE);
                result = NULL;
            }
          break_switch:;
        }

        /*
         *  If the match failed and there's a backtrack option, take it.
         *  Otherwise this is a complete and utter failure.
         */
        if (!result) {
            if (gData->cursz == 0)
                return NULL;

            /* Potentially detect explosive regex here. */
            gData->backTrackCount++;
            if (gData->backTrackLimit &&
                gData->backTrackCount >= gData->backTrackLimit) {
                JS_ReportErrorNumber(gData->cx, js_GetErrorMessage, NULL,
                                     JSMSG_REGEXP_TOO_COMPLEX);
                gData->ok = FALSE;
                return NULL;
            }

            backTrackData = gData->backTrackSP;
            gData->cursz = backTrackData->sz;
            gData->backTrackSP =
                (REBackTrackData *) ((char *)backTrackData - backTrackData->sz);
            x->cp = backTrackData->cp;
            pc = backTrackData->backtrack_pc;
            op = (REOp) backTrackData->backtrack_op;
            assert(op < REOP_LIMIT);
            gData->stateStackTop = backTrackData->saveStateStackTop;
            assert(gData->stateStackTop);

            memcpy(gData->stateStack, backTrackData + 1,
                   sizeof(REProgState) * backTrackData->saveStateStackTop);
            curState = &gData->stateStack[gData->stateStackTop - 1];

            if (backTrackData->parenCount) {
                memcpy(&x->parens[backTrackData->parenIndex],
                       (char *)(backTrackData + 1) +
                       sizeof(REProgState) * backTrackData->saveStateStackTop,
                       sizeof(RECapture) * backTrackData->parenCount);
                parenSoFar = backTrackData->parenIndex + backTrackData->parenCount;
            } else {
                for (k = curState->parenSoFar; k < parenSoFar; k++)
                    x->parens[k].index = -1;
                parenSoFar = curState->parenSoFar;
            }

            TRACE("\tBT_Pop: %Id,%Id\n",
                     (ULONG_PTR)backTrackData->parenIndex,
                     (ULONG_PTR)backTrackData->parenCount);
            continue;
        }
        x = result;

        /*
         *  Continue with the expression.
         */
        op = (REOp)*pc++;
        assert(op < REOP_LIMIT);
    }

bad:
    TRACE("\n");
    return NULL;

good:
    TRACE("\n");
    return x;
}

static match_state_t *MatchRegExp(REGlobalData *gData, match_state_t *x)
{
    match_state_t *result;
    const WCHAR *cp = x->cp;
    const WCHAR *cp2;
    UINT j;

    /*
     * Have to include the position beyond the last character
     * in order to detect end-of-input/line condition.
     */
    for (cp2 = cp; cp2 <= gData->cpend; cp2++) {
        gData->skipped = cp2 - cp;
        x->cp = cp2;
        for (j = 0; j < gData->regexp->parenCount; j++)
            x->parens[j].index = -1;
        result = ExecuteREBytecode(gData, x);
        if (!gData->ok || result || (gData->regexp->flags & REG_STICKY))
            return result;
        gData->backTrackSP = gData->backTrackStack;
        gData->cursz = 0;
        gData->stateStackTop = 0;
        cp2 = cp + gData->skipped;
    }
    return NULL;
}

static HRESULT InitMatch(regexp_t *re, void *cx, heap_pool_t *pool, REGlobalData *gData)
{
    UINT i;

    gData->backTrackStackSize = INITIAL_BACKTRACK;
    gData->backTrackStack = heap_pool_alloc(gData->pool, INITIAL_BACKTRACK);
    if (!gData->backTrackStack)
        goto bad;

    gData->backTrackSP = gData->backTrackStack;
    gData->cursz = 0;
    gData->backTrackCount = 0;
    gData->backTrackLimit = 0;

    gData->stateStackLimit = INITIAL_STATESTACK;
    gData->stateStack = heap_pool_alloc(gData->pool, sizeof(REProgState) * INITIAL_STATESTACK);
    if (!gData->stateStack)
        goto bad;

    gData->stateStackTop = 0;
    gData->cx = cx;
    gData->pool = pool;
    gData->regexp = re;
    gData->ok = TRUE;

    for (i = 0; i < re->classCount; i++) {
        if (!re->classList[i].converted &&
                !ProcessCharSet(gData, &re->classList[i])) {
            return E_FAIL;
        }
    }

    return S_OK;

bad:
    gData->ok = FALSE;
    return E_OUTOFMEMORY;
}

HRESULT regexp_execute(regexp_t *regexp, void *cx, heap_pool_t *pool,
        const WCHAR *str, DWORD str_len, match_state_t *result)
{
    match_state_t *res;
    REGlobalData gData;
    heap_pool_t *mark = heap_pool_mark(pool);
    const WCHAR *str_beg = result->cp;
    HRESULT hres;

    assert(result->cp != NULL);

    gData.cpbegin = str;
    gData.cpend = str+str_len;
    gData.start = result->cp-str;
    gData.skipped = 0;
    gData.pool = pool;

    hres = InitMatch(regexp, cx, pool, &gData);
    if(FAILED(hres)) {
        WARN("InitMatch failed\n");
        heap_pool_clear(mark);
        return hres;
    }

    res = MatchRegExp(&gData, result);
    heap_pool_clear(mark);
    if(!gData.ok) {
        WARN("MatchRegExp failed\n");
        return E_FAIL;
    }

    if(!res) {
        result->match_len = 0;
        return S_FALSE;
    }

    result->match_len = (result->cp-str_beg) - gData.skipped;
    result->paren_count = regexp->parenCount;
    return S_OK;
}

void regexp_destroy(regexp_t *re)
{
    if (re->classList) {
        UINT i;
        for (i = 0; i < re->classCount; i++) {
            if (re->classList[i].converted)
                free(re->classList[i].u.bits);
            re->classList[i].u.bits = NULL;
        }
        free(re->classList);
    }
    free(re);
}

regexp_t* regexp_new(void *cx, heap_pool_t *pool, const WCHAR *str,
        DWORD str_len, WORD flags, BOOL flat)
{
    regexp_t *re;
    heap_pool_t *mark;
    CompilerState state;
    size_t resize;
    jsbytecode *endPC;
    UINT i;
    size_t len;

    re = NULL;
    mark = heap_pool_mark(pool);
    len = str_len;

    state.context = cx;
    state.pool = pool;
    state.cp = str;
    if (!state.cp)
        goto out;
    state.cpbegin = state.cp;
    state.cpend = state.cp + len;
    state.flags = flags;
    state.parenCount = 0;
    state.classCount = 0;
    state.progLength = 0;
    state.treeDepth = 0;
    state.classBitmapsMem = 0;
    for (i = 0; i < CLASS_CACHE_SIZE; i++)
        state.classCache[i].start = NULL;

    if (len != 0 && flat) {
        state.result = NewRENode(&state, REOP_FLAT);
        if (!state.result)
            goto out;
        state.result->u.flat.chr = *state.cpbegin;
        state.result->u.flat.length = len;
        state.result->kid = (void *) state.cpbegin;
        /* Flat bytecode: REOP_FLAT compact(string_offset) compact(len). */
        state.progLength += 1 + GetCompactIndexWidth(0)
                          + GetCompactIndexWidth(len);
    } else {
        if (!ParseRegExp(&state))
            goto out;
    }
    resize = offsetof(regexp_t, program) + state.progLength + 1;
    re = malloc(resize);
    if (!re)
        goto out;

    assert(state.classBitmapsMem <= CLASS_BITMAPS_MEM_LIMIT);
    re->classCount = state.classCount;
    if (re->classCount) {
        re->classList = malloc(re->classCount * sizeof(RECharSet));
        if (!re->classList) {
            regexp_destroy(re);
            re = NULL;
            goto out;
        }
        for (i = 0; i < re->classCount; i++)
            re->classList[i].converted = FALSE;
    } else {
        re->classList = NULL;
    }
    endPC = EmitREBytecode(&state, re, state.treeDepth, re->program, state.result);
    if (!endPC) {
        regexp_destroy(re);
        re = NULL;
        goto out;
    }
    *endPC++ = REOP_END;
    /*
     * Check whether size was overestimated and shrink using realloc.
     * This is safe since no pointers to newly parsed regexp or its parts
     * besides re exist here.
     */
    if ((size_t)(endPC - re->program) != state.progLength + 1) {
        regexp_t *tmp;
        assert((size_t)(endPC - re->program) < state.progLength + 1);
        resize = offsetof(regexp_t, program) + (endPC - re->program);
        tmp = realloc(re, resize);
        if (tmp)
            re = tmp;
    }

    re->flags = flags;
    re->parenCount = state.parenCount;
    re->source = str;
    re->source_len = str_len;

out:
    heap_pool_clear(mark);
    return re;
}
