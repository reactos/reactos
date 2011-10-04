/*
*******************************************************************************
*
*   Copyright (C) 2003-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  uit_len8.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003feb10
*   created by: Markus W. Scherer
*
*   This file contains the implementation of the "lenient UTF-8" UCharIterator
*   as used in the uciter8 sample code.
*   UTF-8-style macros are defined as well as the UCharIterator.
*   The macros are incomplete (do not assemble code points from pairs of
*   surrogates, see comment below)
*   but sufficient for the iterator.
*/

#include <string.h>
#include "unicode/utypes.h"
#include "unicode/uiter.h"

/* lenient UTF-8/CESU-8 macros ---------------------------------------------- */

/*
 * This code leniently reads 8-bit Unicode strings,
 * which could contain a mix of UTF-8 and CESU-8.
 * More precisely:
 * - supplementary code points may be encoded with dedicated 4-byte sequences
 *   (UTF-8 style)
 * - supplementary code points may be encoded with
 *   pairs of 3-byte sequences, one for each surrogate of the UTF-16 form
 *   (CESU-8 style)
 * - single surrogates are allowed, encoded with their "natural" 3-byte sequences
 *
 * Limitation:
 * Right now, the macros do not attempt to assemble code points from pairs of
 * separately encoded surrogates.
 * This would not be sufficient for processing based on these macros,
 * but it is sufficient for a UCharIterator that returns only UChars anyway.
 *
 * The code is copied and modified from utf_impl.c and utf8.h.
 *
 * Change 2006feb08: Much of the implementation code is replaced by calling
 * the utf_impl.c functions which accept a new "strict" parameter value
 * of -2 implementing exactly this leniency.
 */

#define L8_NEXT(s, i, length, c) { \
    (c)=(uint8_t)(s)[(i)++]; \
    if((c)>=0x80) { \
        if(U8_IS_LEAD(c)) { \
            (c)=utf8_nextCharSafeBody((const uint8_t *)s, &(i), (int32_t)(length), c, -2); \
        } else { \
            (c)=U_SENTINEL; \
        } \
    } \
}

#define L8_PREV(s, start, i, c) { \
    (c)=(uint8_t)(s)[--(i)]; \
    if((c)>=0x80) { \
        if((c)<=0xbf) { \
            (c)=utf8_prevCharSafeBody((const uint8_t *)s, start, &(i), c, -2); \
        } else { \
            (c)=U_SENTINEL; \
        } \
    } \
}

/* lenient-8 UCharIterator -------------------------------------------------- */

/*
 * This is a copy of the UTF-8 UCharIterator in uiter.cpp,
 * except that it uses the lenient-8-bit-Unicode macros above.
 */

/*
 * Minimal implementation:
 * Maintain a single-UChar buffer for an additional surrogate.
 * The caller must not modify start and limit because they are used internally.
 *
 * Use UCharIterator fields as follows:
 *   context        pointer to UTF-8 string
 *   length         UTF-16 length of the string; -1 until lazy evaluation
 *   start          current UTF-8 index
 *   index          current UTF-16 index; may be -1="unknown" after setState()
 *   limit          UTF-8 length of the string
 *   reservedField  supplementary code point
 *
 * Since UCharIterator delivers 16-bit code units, the iteration can be
 * currently in the middle of the byte sequence for a supplementary code point.
 * In this case, reservedField will contain that code point and start will
 * point to after the corresponding byte sequence. The UTF-16 index will be
 * one less than what it would otherwise be corresponding to the UTF-8 index.
 * Otherwise, reservedField will be 0.
 */

/*
 * Possible optimization for NUL-terminated UTF-8 and UTF-16 strings:
 * Add implementations that do not call strlen() for iteration but check for NUL.
 */

static int32_t U_CALLCONV
lenient8IteratorGetIndex(UCharIterator *iter, UCharIteratorOrigin origin) {
    switch(origin) {
    case UITER_ZERO:
    case UITER_START:
        return 0;
    case UITER_CURRENT:
        if(iter->index<0) {
            /* the current UTF-16 index is unknown after setState(), count from the beginning */
            const uint8_t *s;
            UChar32 c;
            int32_t i, limit, index;

            s=(const uint8_t *)iter->context;
            i=index=0;
            limit=iter->start; /* count up to the UTF-8 index */
            while(i<limit) {
                L8_NEXT(s, i, limit, c);
                if(c<=0xffff) {
                    ++index;
                } else {
                    index+=2;
                }
            }

            iter->start=i; /* just in case setState() did not get us to a code point boundary */
            if(i==iter->limit) {
                iter->length=index; /* in case it was <0 or wrong */
            }
            if(iter->reservedField!=0) {
                --index; /* we are in the middle of a supplementary code point */
            }
            iter->index=index;
        }
        return iter->index;
    case UITER_LIMIT:
    case UITER_LENGTH:
        if(iter->length<0) {
            const uint8_t *s;
            UChar32 c;
            int32_t i, limit, length;

            s=(const uint8_t *)iter->context;
            if(iter->index<0) {
                /*
                 * the current UTF-16 index is unknown after setState(),
                 * we must first count from the beginning to here
                 */
                i=length=0;
                limit=iter->start;

                /* count from the beginning to the current index */
                while(i<limit) {
                    L8_NEXT(s, i, limit, c);
                    if(c<=0xffff) {
                        ++length;
                    } else {
                        length+=2;
                    }
                }

                /* assume i==limit==iter->start, set the UTF-16 index */
                iter->start=i; /* just in case setState() did not get us to a code point boundary */
                iter->index= iter->reservedField!=0 ? length-1 : length;
            } else {
                i=iter->start;
                length=iter->index;
                if(iter->reservedField!=0) {
                    ++length;
                }
            }

            /* count from the current index to the end */
            limit=iter->limit;
            while(i<limit) {
                L8_NEXT(s, i, limit, c);
                if(c<=0xffff) {
                    ++length;
                } else {
                    length+=2;
                }
            }
            iter->length=length;
        }
        return iter->length;
    default:
        /* not a valid origin */
        /* Should never get here! */
        return -1;
    }
}

static int32_t U_CALLCONV
lenient8IteratorMove(UCharIterator *iter, int32_t delta, UCharIteratorOrigin origin) {
    const uint8_t *s;
    UChar32 c;
    int32_t pos; /* requested UTF-16 index */
    int32_t i; /* UTF-8 index */
    UBool havePos;

    /* calculate the requested UTF-16 index */
    switch(origin) {
    case UITER_ZERO:
    case UITER_START:
        pos=delta;
        havePos=TRUE;
        /* iter->index<0 (unknown) is possible */
        break;
    case UITER_CURRENT:
        if(iter->index>=0) {
            pos=iter->index+delta;
            havePos=TRUE;
        } else {
            /* the current UTF-16 index is unknown after setState(), use only delta */
            pos=0;
            havePos=FALSE;
        }
        break;
    case UITER_LIMIT:
    case UITER_LENGTH:
        if(iter->length>=0) {
            pos=iter->length+delta;
            havePos=TRUE;
        } else {
            /* pin to the end, avoid counting the length */
            iter->index=-1;
            iter->start=iter->limit;
            iter->reservedField=0;
            if(delta>=0) {
                return UITER_UNKNOWN_INDEX;
            } else {
                /* the current UTF-16 index is unknown, use only delta */
                pos=0;
                havePos=FALSE;
            }
        }
        break;
    default:
        return -1;  /* Error */
    }

    if(havePos) {
        /* shortcuts: pinning to the edges of the string */
        if(pos<=0) {
            iter->index=iter->start=iter->reservedField=0;
            return 0;
        } else if(iter->length>=0 && pos>=iter->length) {
            iter->index=iter->length;
            iter->start=iter->limit;
            iter->reservedField=0;
            return iter->index;
        }

        /* minimize the number of L8_NEXT/PREV operations */
        if(iter->index<0 || pos<iter->index/2) {
            /* go forward from the start instead of backward from the current index */
            iter->index=iter->start=iter->reservedField=0;
        } else if(iter->length>=0 && (iter->length-pos)<(pos-iter->index)) {
            /*
             * if we have the UTF-16 index and length and the new position is
             * closer to the end than the current index,
             * then go backward from the end instead of forward from the current index
             */
            iter->index=iter->length;
            iter->start=iter->limit;
            iter->reservedField=0;
        }

        delta=pos-iter->index;
        if(delta==0) {
            return iter->index; /* nothing to do */
        }
    } else {
        /* move relative to unknown UTF-16 index */
        if(delta==0) {
            return UITER_UNKNOWN_INDEX; /* nothing to do */
        } else if(-delta>=iter->start) {
            /* moving backwards by more UChars than there are UTF-8 bytes, pin to 0 */
            iter->index=iter->start=iter->reservedField=0;
            return 0;
        } else if(delta>=(iter->limit-iter->start)) {
            /* moving forward by more UChars than the remaining UTF-8 bytes, pin to the end */
            iter->index=iter->length; /* may or may not be <0 (unknown) */
            iter->start=iter->limit;
            iter->reservedField=0;
            return iter->index>=0 ? iter->index : UITER_UNKNOWN_INDEX;
        }
    }

    /* delta!=0 */

    /* move towards the requested position, pin to the edges of the string */
    s=(const uint8_t *)iter->context;
    pos=iter->index; /* could be <0 (unknown) */
    i=iter->start;
    if(delta>0) {
        /* go forward */
        int32_t limit=iter->limit;
        if(iter->reservedField!=0) {
            iter->reservedField=0;
            ++pos;
            --delta;
        }
        while(delta>0 && i<limit) {
            L8_NEXT(s, i, limit, c);
            if(c<0xffff) {
                ++pos;
                --delta;
            } else if(delta>=2) {
                pos+=2;
                delta-=2;
            } else /* delta==1 */ {
                /* stop in the middle of a supplementary code point */
                iter->reservedField=c;
                ++pos;
                break; /* delta=0; */
            }
        }
        if(i==limit) {
            if(iter->length<0 && iter->index>=0) {
                iter->length= iter->reservedField==0 ? pos : pos+1;
            } else if(iter->index<0 && iter->length>=0) {
                iter->index= iter->reservedField==0 ? iter->length : iter->length-1;
            }
        }
    } else /* delta<0 */ {
        /* go backward */
        if(iter->reservedField!=0) {
            iter->reservedField=0;
            i-=4; /* we stayed behind the supplementary code point; go before it now */
            --pos;
            ++delta;
        }
        while(delta<0 && i>0) {
            L8_PREV(s, 0, i, c);
            if(c<0xffff) {
                --pos;
                ++delta;
            } else if(delta<=-2) {
                pos-=2;
                delta+=2;
            } else /* delta==-1 */ {
                /* stop in the middle of a supplementary code point */
                i+=4; /* back to behind this supplementary code point for consistent state */
                iter->reservedField=c;
                --pos;
                break; /* delta=0; */
            }
        }
    }

    iter->start=i;
    if(iter->index>=0) {
        return iter->index=pos;
    } else {
        /* we started with index<0 (unknown) so pos is bogus */
        if(i<=1) {
            return iter->index=i; /* reached the beginning */
        } else {
            /* we still don't know the UTF-16 index */
            return UITER_UNKNOWN_INDEX;
        }
    }
}

static UBool U_CALLCONV
lenient8IteratorHasNext(UCharIterator *iter) {
    return iter->reservedField!=0 || iter->start<iter->limit;
}

static UBool U_CALLCONV
lenient8IteratorHasPrevious(UCharIterator *iter) {
    return iter->start>0;
}

static UChar32 U_CALLCONV
lenient8IteratorCurrent(UCharIterator *iter) {
    if(iter->reservedField!=0) {
        return U16_TRAIL(iter->reservedField);
    } else if(iter->start<iter->limit) {
        const uint8_t *s=(const uint8_t *)iter->context;
        UChar32 c;
        int32_t i=iter->start;

        L8_NEXT(s, i, iter->limit, c);
        if(c<0) {
            return 0xfffd;
        } else if(c<=0xffff) {
            return c;
        } else {
            return U16_LEAD(c);
        }
    } else {
        return U_SENTINEL;
    }
}

static UChar32 U_CALLCONV
lenient8IteratorNext(UCharIterator *iter) {
    int32_t index;

    if(iter->reservedField!=0) {
        UChar trail=U16_TRAIL(iter->reservedField);
        iter->reservedField=0;
        if((index=iter->index)>=0) {
            iter->index=index+1;
        }
        return trail;
    } else if(iter->start<iter->limit) {
        const uint8_t *s=(const uint8_t *)iter->context;
        UChar32 c;

        L8_NEXT(s, iter->start, iter->limit, c);
        if((index=iter->index)>=0) {
            iter->index=++index;
            if(iter->length<0 && iter->start==iter->limit) {
                iter->length= c<=0xffff ? index : index+1;
            }
        } else if(iter->start==iter->limit && iter->length>=0) {
            iter->index= c<=0xffff ? iter->length : iter->length-1;
        }
        if(c<0) {
            return 0xfffd;
        } else if(c<=0xffff) {
            return c;
        } else {
            iter->reservedField=c;
            return U16_LEAD(c);
        }
    } else {
        return U_SENTINEL;
    }
}

static UChar32 U_CALLCONV
lenient8IteratorPrevious(UCharIterator *iter) {
    int32_t index;

    if(iter->reservedField!=0) {
        UChar lead=U16_LEAD(iter->reservedField);
        iter->reservedField=0;
        iter->start-=4; /* we stayed behind the supplementary code point; go before it now */
        if((index=iter->index)>0) {
            iter->index=index-1;
        }
        return lead;
    } else if(iter->start>0) {
        const uint8_t *s=(const uint8_t *)iter->context;
        UChar32 c;

        L8_PREV(s, 0, iter->start, c);
        if((index=iter->index)>0) {
            iter->index=index-1;
        } else if(iter->start<=1) {
            iter->index= c<=0xffff ? iter->start : iter->start+1;
        }
        if(c<0) {
            return 0xfffd;
        } else if(c<=0xffff) {
            return c;
        } else {
            iter->start+=4; /* back to behind this supplementary code point for consistent state */
            iter->reservedField=c;
            return U16_TRAIL(c);
        }
    } else {
        return U_SENTINEL;
    }
}

static uint32_t U_CALLCONV
lenient8IteratorGetState(const UCharIterator *iter) {
    uint32_t state=(uint32_t)(iter->start<<1);
    if(iter->reservedField!=0) {
        state|=1;
    }
    return state;
}

static void U_CALLCONV
lenient8IteratorSetState(UCharIterator *iter, uint32_t state, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        /* do nothing */
    } else if(iter==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    } else if(state==lenient8IteratorGetState(iter)) {
        /* setting to the current state: no-op */
    } else {
        int32_t index=(int32_t)(state>>1); /* UTF-8 index */
        state&=1; /* 1 if in surrogate pair, must be index>=4 */

        if((state==0 ? index<0 : index<4) || iter->limit<index) {
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        } else {
            iter->start=index; /* restore UTF-8 byte index */
            if(index<=1) {
                iter->index=index;
            } else {
                iter->index=-1; /* unknown UTF-16 index */
            }
            if(state==0) {
                iter->reservedField=0;
            } else {
                /* verified index>=4 above */
                UChar32 c;
                L8_PREV((const uint8_t *)iter->context, 0, index, c);
                if(c<=0xffff) {
                    *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                } else {
                    iter->reservedField=c;
                }
            }
        }
    }
}

static const UCharIterator lenient8Iterator={
    0, 0, 0, 0, 0, 0,
    lenient8IteratorGetIndex,
    lenient8IteratorMove,
    lenient8IteratorHasNext,
    lenient8IteratorHasPrevious,
    lenient8IteratorCurrent,
    lenient8IteratorNext,
    lenient8IteratorPrevious,
    NULL,
    lenient8IteratorGetState,
    lenient8IteratorSetState
};

U_CAPI void U_EXPORT2
uiter_setLenient8(UCharIterator *iter, const char *s, int32_t length) {
    if(iter!=0) {
        if(s!=0 && length>=-1) {
            *iter=lenient8Iterator;
            iter->context=s;
            if(length>=0) {
                iter->limit=length;
            } else {
                iter->limit=strlen(s);
            }
            iter->length= iter->limit<=1 ? iter->limit : -1;
        } else {
            /* set no-op iterator */
            uiter_setString(iter, NULL, 0);
        }
    }
}
