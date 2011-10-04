/*
 *************************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1996-2005, International Business Machines Corporation and
 * others. All Rights Reserved.
 *************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/unistr.h"
#include "unicode/chariter.h"
#include "unicode/schriter.h"
#include "unicode/uchriter.h"
#include "unicode/uiter.h"
#include "unicode/normlzr.h"
#include "cmemory.h"
#include "unormimp.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(Normalizer)

//-------------------------------------------------------------------------
// Constructors and other boilerplate
//-------------------------------------------------------------------------

Normalizer::Normalizer(const UnicodeString& str, UNormalizationMode mode) :
    UObject(), fUMode(mode), fOptions(0),
    currentIndex(0), nextIndex(0),
    buffer(), bufferPos(0)
{
    init(new StringCharacterIterator(str));
}

Normalizer::Normalizer(const UChar *str, int32_t length, UNormalizationMode mode) :
    UObject(), fUMode(mode), fOptions(0),
    currentIndex(0), nextIndex(0),
    buffer(), bufferPos(0)
{
    init(new UCharCharacterIterator(str, length));
}

Normalizer::Normalizer(const CharacterIterator& iter, UNormalizationMode mode) :
    UObject(), fUMode(mode), fOptions(0),
    currentIndex(0), nextIndex(0),
    buffer(), bufferPos(0)
{
    init(iter.clone());
}

Normalizer::Normalizer(const Normalizer &copy) :
    UObject(copy), fUMode(copy.fUMode), fOptions(copy.fOptions),
    currentIndex(copy.currentIndex), nextIndex(copy.nextIndex),
    buffer(copy.buffer), bufferPos(copy.bufferPos)
{
    init(((CharacterIterator *)(copy.text->context))->clone());
}

static const UChar _NUL=0;

void
Normalizer::init(CharacterIterator *iter) {
    UErrorCode errorCode=U_ZERO_ERROR;

    text=(UCharIterator *)uprv_malloc(sizeof(UCharIterator));
    if(text!=NULL) {
        if(unorm_haveData(&errorCode)) {
            uiter_setCharacterIterator(text, iter);
        } else {
            delete iter;
            uiter_setCharacterIterator(text, new UCharCharacterIterator(&_NUL, 0));
        }
    } else {
        delete iter;
    }
}

Normalizer::~Normalizer()
{
    if(text!=NULL) {
        delete (CharacterIterator *)text->context;
        uprv_free(text);
    }
}

Normalizer* 
Normalizer::clone() const
{
    if(this!=0) {
        return new Normalizer(*this);
    } else {
        return 0;
    }
}

/**
 * Generates a hash code for this iterator.
 */
int32_t Normalizer::hashCode() const
{
    return ((CharacterIterator *)(text->context))->hashCode() + fUMode + fOptions + buffer.hashCode() + bufferPos + currentIndex + nextIndex;
}
    
UBool Normalizer::operator==(const Normalizer& that) const
{
    return
        this==&that ||
        fUMode==that.fUMode &&
        fOptions==that.fOptions &&
        *((CharacterIterator *)(text->context))==*((CharacterIterator *)(that.text->context)) &&
        buffer==that.buffer &&
        bufferPos==that.bufferPos &&
        nextIndex==that.nextIndex;
}

//-------------------------------------------------------------------------
// Static utility methods
//-------------------------------------------------------------------------

void U_EXPORT2
Normalizer::normalize(const UnicodeString& source, 
                      UNormalizationMode mode, int32_t options,
                      UnicodeString& result, 
                      UErrorCode &status) {
    if(source.isBogus() || U_FAILURE(status)) {
        result.setToBogus();
        if(U_SUCCESS(status)) {
            status=U_ILLEGAL_ARGUMENT_ERROR;
        }
    } else {
        UnicodeString localDest;
        UnicodeString *dest;

        if(&source!=&result) {
            dest=&result;
        } else {
            // the source and result strings are the same object, use a temporary one
            dest=&localDest;
        }

        UChar *buffer=dest->getBuffer(source.length());
        int32_t length=unorm_internalNormalize(buffer, dest->getCapacity(),
                                               source.getBuffer(), source.length(),
                                               mode, options,
                                               &status);
        dest->releaseBuffer(U_SUCCESS(status) ? length : 0);
        if(status==U_BUFFER_OVERFLOW_ERROR) {
            status=U_ZERO_ERROR;
            buffer=dest->getBuffer(length);
            length=unorm_internalNormalize(buffer, dest->getCapacity(),
                                           source.getBuffer(), source.length(),
                                           mode, options,
                                           &status);
            dest->releaseBuffer(U_SUCCESS(status) ? length : 0);
        }

        if(dest==&localDest) {
            result=*dest;
        }
        if(U_FAILURE(status)) {
            result.setToBogus();
        }
    }
}

void U_EXPORT2
Normalizer::compose(const UnicodeString& source, 
                    UBool compat, int32_t options,
                    UnicodeString& result, 
                    UErrorCode &status) {
    if(source.isBogus() || U_FAILURE(status)) {
        result.setToBogus();
        if(U_SUCCESS(status)) {
            status=U_ILLEGAL_ARGUMENT_ERROR;
        }
    } else {
        UnicodeString localDest;
        UnicodeString *dest;

        if(&source!=&result) {
            dest=&result;
        } else {
            // the source and result strings are the same object, use a temporary one
            dest=&localDest;
        }

        UChar *buffer=dest->getBuffer(source.length());
        int32_t length=unorm_compose(buffer, dest->getCapacity(),
                                     source.getBuffer(), source.length(),
                                     compat, options,
                                     &status);
        dest->releaseBuffer(U_SUCCESS(status) ? length : 0);
        if(status==U_BUFFER_OVERFLOW_ERROR) {
            status=U_ZERO_ERROR;
            buffer=dest->getBuffer(length);
            length=unorm_compose(buffer, dest->getCapacity(),
                                 source.getBuffer(), source.length(),
                                 compat, options,
                                 &status);
            dest->releaseBuffer(U_SUCCESS(status) ? length : 0);
        }

        if(dest==&localDest) {
            result=*dest;
        }
        if(U_FAILURE(status)) {
            result.setToBogus();
        }
    }
}

void U_EXPORT2
Normalizer::decompose(const UnicodeString& source, 
                      UBool compat, int32_t options,
                      UnicodeString& result, 
                      UErrorCode &status) {
    if(source.isBogus() || U_FAILURE(status)) {
        result.setToBogus();
        if(U_SUCCESS(status)) {
            status=U_ILLEGAL_ARGUMENT_ERROR;
        }
    } else {
        UnicodeString localDest;
        UnicodeString *dest;

        if(&source!=&result) {
            dest=&result;
        } else {
            // the source and result strings are the same object, use a temporary one
            dest=&localDest;
        }

        UChar *buffer=dest->getBuffer(source.length());
        int32_t length=unorm_decompose(buffer, dest->getCapacity(),
                                     source.getBuffer(), source.length(),
                                     compat, options,
                                     &status);
        dest->releaseBuffer(U_SUCCESS(status) ? length : 0);
        if(status==U_BUFFER_OVERFLOW_ERROR) {
            status=U_ZERO_ERROR;
            buffer=dest->getBuffer(length);
            length=unorm_decompose(buffer, dest->getCapacity(),
                                   source.getBuffer(), source.length(),
                                   compat, options,
                                   &status);
            dest->releaseBuffer(U_SUCCESS(status) ? length : 0);
        }

        if(dest==&localDest) {
            result=*dest;
        }
        if(U_FAILURE(status)) {
            result.setToBogus();
        }
    }
}

UnicodeString & U_EXPORT2
Normalizer::concatenate(UnicodeString &left, UnicodeString &right,
                        UnicodeString &result,
                        UNormalizationMode mode, int32_t options,
                        UErrorCode &errorCode) {
    if(left.isBogus() || right.isBogus() || U_FAILURE(errorCode)) {
        result.setToBogus();
        if(U_SUCCESS(errorCode)) {
            errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        }
    } else {
        UnicodeString localDest;
        UnicodeString *dest;

        if(&left!=&result && &right!=&result) {
            dest=&result;
        } else {
            // the source and result strings are the same object, use a temporary one
            dest=&localDest;
        }

        UChar *buffer=dest->getBuffer(left.length()+right.length());
        int32_t length=unorm_concatenate(left.getBuffer(), left.length(),
                                         right.getBuffer(), right.length(),
                                         buffer, dest->getCapacity(),
                                         mode, options,
                                         &errorCode);
        dest->releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
        if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
            errorCode=U_ZERO_ERROR;
            buffer=dest->getBuffer(length);
            int32_t length=unorm_concatenate(left.getBuffer(), left.length(),
                                             right.getBuffer(), right.length(),
                                             buffer, dest->getCapacity(),
                                             mode, options,
                                             &errorCode);
            dest->releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
        }

        if(dest==&localDest) {
            result=*dest;
        }
        if(U_FAILURE(errorCode)) {
            result.setToBogus();
        }
    }
    return result;
}

//-------------------------------------------------------------------------
// Iteration API
//-------------------------------------------------------------------------

/**
 * Return the current character in the normalized text.
 */
UChar32 Normalizer::current() {
    if(bufferPos<buffer.length() || nextNormalize()) {
        return buffer.char32At(bufferPos);
    } else {
        return DONE;
    }
}

/**
 * Return the next character in the normalized text and advance
 * the iteration position by one.  If the end
 * of the text has already been reached, {@link #DONE} is returned.
 */
UChar32 Normalizer::next() {
    if(bufferPos<buffer.length() ||  nextNormalize()) {
        UChar32 c=buffer.char32At(bufferPos);
        bufferPos+=UTF_CHAR_LENGTH(c);
        return c;
    } else {
        return DONE;
    }
}

/**
 * Return the previous character in the normalized text and decrement
 * the iteration position by one.  If the beginning
 * of the text has already been reached, {@link #DONE} is returned.
 */
UChar32 Normalizer::previous() {
    if(bufferPos>0 || previousNormalize()) {
        UChar32 c=buffer.char32At(bufferPos-1);
        bufferPos-=UTF_CHAR_LENGTH(c);
        return c;
    } else {
        return DONE;
    }
}

void Normalizer::reset() {
    currentIndex=nextIndex=text->move(text, 0, UITER_START);
    clearBuffer();
}

void
Normalizer::setIndexOnly(int32_t index) {
    currentIndex=nextIndex=text->move(text, index, UITER_ZERO); // validates index
    clearBuffer();
}

/**
 * Return the first character in the normalized text->  This resets
 * the <tt>Normalizer's</tt> position to the beginning of the text->
 */
UChar32 Normalizer::first() {
    reset();
    return next();
}

/**
 * Return the last character in the normalized text->  This resets
 * the <tt>Normalizer's</tt> position to be just before the
 * the input text corresponding to that normalized character.
 */
UChar32 Normalizer::last() {
    currentIndex=nextIndex=text->move(text, 0, UITER_LIMIT);
    clearBuffer();
    return previous();
}

/**
 * Retrieve the current iteration position in the input text that is
 * being normalized.  This method is useful in applications such as
 * searching, where you need to be able to determine the position in
 * the input text that corresponds to a given normalized output character.
 * <p>
 * <b>Note:</b> This method sets the position in the <em>input</em>, while
 * {@link #next} and {@link #previous} iterate through characters in the
 * <em>output</em>.  This means that there is not necessarily a one-to-one
 * correspondence between characters returned by <tt>next</tt> and
 * <tt>previous</tt> and the indices passed to and returned from
 * <tt>setIndex</tt> and {@link #getIndex}.
 *
 */
int32_t Normalizer::getIndex() const {
    if(bufferPos<buffer.length()) {
        return currentIndex;
    } else {
        return nextIndex;
    }
}

/**
 * Retrieve the index of the start of the input text->  This is the begin index
 * of the <tt>CharacterIterator</tt> or the start (i.e. 0) of the <tt>String</tt>
 * over which this <tt>Normalizer</tt> is iterating
 */
int32_t Normalizer::startIndex() const {
    return text->getIndex(text, UITER_START);
}

/**
 * Retrieve the index of the end of the input text->  This is the end index
 * of the <tt>CharacterIterator</tt> or the length of the <tt>String</tt>
 * over which this <tt>Normalizer</tt> is iterating
 */
int32_t Normalizer::endIndex() const {
    return text->getIndex(text, UITER_LIMIT);
}

//-------------------------------------------------------------------------
// Property access methods
//-------------------------------------------------------------------------

void
Normalizer::setMode(UNormalizationMode newMode) 
{
    fUMode = newMode;
}

UNormalizationMode
Normalizer::getUMode() const
{
    return fUMode;
}

void
Normalizer::setOption(int32_t option, 
                      UBool value) 
{
    if (value) {
        fOptions |= option;
    } else {
        fOptions &= (~option);
    }
}

UBool
Normalizer::getOption(int32_t option) const
{
    return (fOptions & option) != 0;
}

/**
 * Set the input text over which this <tt>Normalizer</tt> will iterate.
 * The iteration position is set to the beginning of the input text->
 */
void
Normalizer::setText(const UnicodeString& newText, 
                    UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }
    CharacterIterator *newIter = new StringCharacterIterator(newText);
    if (newIter == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    delete (CharacterIterator *)(text->context);
    text->context = newIter;
    reset();
}

/**
 * Set the input text over which this <tt>Normalizer</tt> will iterate.
 * The iteration position is set to the beginning of the string.
 */
void
Normalizer::setText(const CharacterIterator& newText, 
                    UErrorCode &status) 
{
    if (U_FAILURE(status)) {
        return;
    }
    CharacterIterator *newIter = newText.clone();
    if (newIter == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    delete (CharacterIterator *)(text->context);
    text->context = newIter;
    reset();
}

void
Normalizer::setText(const UChar* newText,
                    int32_t length,
                    UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }
    CharacterIterator *newIter = new UCharCharacterIterator(newText, length);
    if (newIter == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    delete (CharacterIterator *)(text->context);
    text->context = newIter;
    reset();
}

/**
 * Copies the text under iteration into the UnicodeString referred to by "result".
 * @param result Receives a copy of the text under iteration.
 */
void
Normalizer::getText(UnicodeString&  result) 
{
    ((CharacterIterator *)(text->context))->getText(result);
}

//-------------------------------------------------------------------------
// Private utility methods
//-------------------------------------------------------------------------

void Normalizer::clearBuffer() {
    buffer.remove();
    bufferPos=0;
}

UBool
Normalizer::nextNormalize() {
    UChar *p;
    int32_t length;
    UErrorCode errorCode;

    clearBuffer();
    currentIndex=nextIndex;
    text->move(text, nextIndex, UITER_ZERO);
    if(!text->hasNext(text)) {
        return FALSE;
    }

    errorCode=U_ZERO_ERROR;
    p=buffer.getBuffer(-1);
    length=unorm_next(text, p, buffer.getCapacity(),
                      fUMode, fOptions,
                      TRUE, 0,
                      &errorCode);
    buffer.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        errorCode=U_ZERO_ERROR;
        text->move(text, nextIndex, UITER_ZERO);
        p=buffer.getBuffer(length);
        length=unorm_next(text, p, buffer.getCapacity(),
                          fUMode, fOptions,
                          TRUE, 0,
                          &errorCode);
        buffer.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    nextIndex=text->getIndex(text, UITER_CURRENT);
    return U_SUCCESS(errorCode) && !buffer.isEmpty();
}

UBool
Normalizer::previousNormalize() {
    UChar *p;
    int32_t length;
    UErrorCode errorCode;

    clearBuffer();
    nextIndex=currentIndex;
    text->move(text, currentIndex, UITER_ZERO);
    if(!text->hasPrevious(text)) {
        return FALSE;
    }

    errorCode=U_ZERO_ERROR;
    p=buffer.getBuffer(-1);
    length=unorm_previous(text, p, buffer.getCapacity(),
                          fUMode, fOptions,
                          TRUE, 0,
                          &errorCode);
    buffer.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        errorCode=U_ZERO_ERROR;
        text->move(text, currentIndex, UITER_ZERO);
        p=buffer.getBuffer(length);
        length=unorm_previous(text, p, buffer.getCapacity(),
                              fUMode, fOptions,
                              TRUE, 0,
                              &errorCode);
        buffer.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    bufferPos=buffer.length();
    currentIndex=text->getIndex(text, UITER_CURRENT);
    return U_SUCCESS(errorCode) && !buffer.isEmpty();
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_NORMALIZATION */
