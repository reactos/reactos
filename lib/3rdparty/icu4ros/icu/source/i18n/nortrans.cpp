/*
**********************************************************************
*   Copyright (C) 2001-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   07/03/01    aliu        Creation.
**********************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uniset.h"
#include "unicode/uiter.h"
#include "nortrans.h"
#include "unormimp.h"
#include "ucln_in.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(NormalizationTransliterator)

/**
 * System registration hook.
 */
void NormalizationTransliterator::registerIDs() {
    UErrorCode errorCode = U_ZERO_ERROR;
    if(!unorm_haveData(&errorCode)) {
        return;
    }

    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFC"),
                                     _create, integerToken(UNORM_NFC));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFKC"),
                                     _create, integerToken(UNORM_NFKC));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFD"),
                                     _create, integerToken(UNORM_NFD));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFKD"),
                                     _create, integerToken(UNORM_NFKD));
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("NFC"),
                                            UNICODE_STRING_SIMPLE("NFD"), TRUE);
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("NFKC"),
                                            UNICODE_STRING_SIMPLE("NFKD"), TRUE);
}

/**
 * Factory methods
 */
Transliterator* NormalizationTransliterator::_create(const UnicodeString& ID,
                                                     Token context) {
    return new NormalizationTransliterator(ID, (UNormalizationMode) context.integer, 0);
}

/**
 * Constructs a transliterator.
 */
NormalizationTransliterator::NormalizationTransliterator(
                                 const UnicodeString& id,
                                 UNormalizationMode mode, int32_t opt) :
    Transliterator(id, 0) {
    fMode = mode;
    options = opt;
}

/**
 * Destructor.
 */
NormalizationTransliterator::~NormalizationTransliterator() {
}

/**
 * Copy constructor.
 */
NormalizationTransliterator::NormalizationTransliterator(const NormalizationTransliterator& o) :
Transliterator(o) {
    fMode = o.fMode;
    options = o.options;
}

/**
 * Assignment operator.
 */
/*NormalizationTransliterator& NormalizationTransliterator::operator=(const NormalizationTransliterator& o) {
    Transliterator::operator=(o);
    fMode = o.fMode;
    options = o.options;
    return *this;
}*/

/**
 * Transliterator API.
 */
Transliterator* NormalizationTransliterator::clone(void) const {
    return new NormalizationTransliterator(*this);
}

/**
 * Implements {@link Transliterator#handleTransliterate}.
 */
void NormalizationTransliterator::handleTransliterate(Replaceable& text, UTransPosition& offsets,
                                                      UBool isIncremental) const {
    // start and limit of the input range
    int32_t start = offsets.start;
    int32_t limit = offsets.limit;
    int32_t length, delta;

    if(start >= limit) {
        return;
    }

    // a C code unit iterator, implemented around the Replaceable
    UCharIterator iter;
    uiter_setReplaceable(&iter, &text);

    // the output string and buffer pointer
    UnicodeString output;
    UChar *buffer;
    UBool neededToNormalize;

    UErrorCode errorCode;

    /*
     * Normalize as short chunks at a time as possible even in
     * bulk mode, so that styled text is minimally disrupted.
     * In incremental mode, a chunk that ends with offsets.limit
     * must not be normalized.
     *
     * If it was known that the input text is not styled, then
     * a bulk mode normalization could look like this:
     *

    UChar staticChars[256];
    UnicodeString input;

    length = limit - start;
    input.setTo(staticChars, 0, sizeof(staticChars)/U_SIZEOF_UCHAR); // writable alias

    _Replaceable_extractBetween(text, start, limit, input.getBuffer(length));
    input.releaseBuffer(length);

    UErrorCode status = U_ZERO_ERROR;
    Normalizer::normalize(input, fMode, options, output, status);

    text.handleReplaceBetween(start, limit, output);

    int32_t delta = output.length() - length;
    offsets.contextLimit += delta;
    offsets.limit += delta;
    offsets.start = limit + delta;

     *
     */
    while(start < limit) {
        // set the iterator limits for the remaining input range
        // this is a moving target because of the replacements in the text object
        iter.start = iter.index = start;
        iter.limit = limit;

        // incrementally normalize a small chunk of the input
        buffer = output.getBuffer(-1);
        errorCode = U_ZERO_ERROR;
        length = unorm_next(&iter, buffer, output.getCapacity(),
                            fMode, 0,
                            TRUE, &neededToNormalize,
                            &errorCode);
        output.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

        if(errorCode == U_BUFFER_OVERFLOW_ERROR) {
            // use a larger output string buffer and do it again from the start
            iter.index = start;
            buffer = output.getBuffer(length);
            errorCode = U_ZERO_ERROR;
            length = unorm_next(&iter, buffer, output.getCapacity(),
                                fMode, 0,
                                TRUE, &neededToNormalize,
                                &errorCode);
            output.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
        }

        if(U_FAILURE(errorCode)) {
            break;
        }

        limit = iter.index;
        if(isIncremental && limit == iter.limit) {
            // stop in incremental mode when we reach the input limit
            // in case there are additional characters that could change the
            // normalization result

            // UNLESS all characters in the result of the normalization of
            // the last run are in the skippable set
            const UChar *s=output.getBuffer();
            int32_t i=0, outLength=output.length();
            UChar32 c;

            while(i<outLength) {
                U16_NEXT(s, i, outLength, c);
                if(!unorm_isNFSkippable(c, fMode)) {
                    outLength=-1; // I wish C++ had labeled loops and break outer; ...
                    break;
                }
            }
            if (outLength<0) {
                break;
            }
        }

        if(neededToNormalize) {
            // replace the input chunk with its normalized form
            text.handleReplaceBetween(start, limit, output);

            // update all necessary indexes accordingly
            delta = length - (limit - start);   // length change in the text object
            start = limit += delta;             // the next chunk starts where this one ends, with adjustment
            limit = offsets.limit += delta;     // set the iteration limit to the adjusted end of the input range
            offsets.contextLimit += delta;
        } else {
            // delta == 0
            start = limit;
            limit = offsets.limit;
        }
    }

    offsets.start = start;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
