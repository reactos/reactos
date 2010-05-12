/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#ifndef __THAISHAPING_H
#define __THAISHAPING_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "OpenTypeTables.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class ThaiShaping /* not : public UObject because all methods are static */ {
public:

    enum {
        // Character classes
        NON =  0,
        CON =  1,
        COA =  2,
        COD =  3,
        LVO =  4,
        FV1 =  5,
        FV2 =  6,
        FV3 =  7,
        BV1 =  8,
        BV2 =  9,
        BDI = 10,
        TON = 11,
        AD1 = 12,
        AD2 = 13,
        AD3 = 14,
        NIK = 15,
        AV1 = 16,
        AV2 = 17,
        AV3 = 18,
        classCount = 19,

        // State Transition actions
        tA  =  0,
        tC  =  1,
        tD  =  2,
        tE  =  3,
        tF  =  4,
        tG  =  5,
        tH  =  6,
        tR  =  7,
        tS  =  8
    };

    struct StateTransition
    {
        le_uint8 nextState;
        le_uint8 action;

        le_uint8 getNextState() { return nextState; };
        le_uint8 getAction() { return action; };
    };

    static le_int32 compose(const LEUnicode *input, le_int32 offset, le_int32 charCount, le_uint8 glyphSet,
        LEUnicode errorChar, LEUnicode *output, LEGlyphStorage &glyphStorage);

private:
    // forbid instantiation
    ThaiShaping();

    static const le_uint8 classTable[];
    static const StateTransition thaiStateTable[][classCount];

    inline static StateTransition getTransition(le_uint8 state, le_uint8 currClass);

    static le_uint8 doTransition(StateTransition transition, LEUnicode currChar, le_int32 inputIndex, le_uint8 glyphSet,
        LEUnicode errorChar, LEUnicode *outputBuffer, LEGlyphStorage &glyphStorage, le_int32 &outputIndex);

    static le_uint8 getNextState(LEUnicode ch, le_uint8 state, le_int32 inputIndex, le_uint8 glyphSet, LEUnicode errorChar,
        le_uint8 &charClass, LEUnicode *output, LEGlyphStorage &glyphStorage, le_int32 &outputIndex);

    static le_bool isLegalHere(LEUnicode ch, le_uint8 prevState);
    static le_uint8 getCharClass(LEUnicode ch);

    static LEUnicode noDescenderCOD(LEUnicode cod, le_uint8 glyphSet);
    static LEUnicode leftAboveVowel(LEUnicode vowel, le_uint8 glyphSet);
    static LEUnicode lowerBelowVowel(LEUnicode vowel, le_uint8 glyphSet);
    static LEUnicode lowerRightTone(LEUnicode tone, le_uint8 glyphSet);
    static LEUnicode lowerLeftTone(LEUnicode tone, le_uint8 glyphSet);
    static LEUnicode upperLeftTone(LEUnicode tone, le_uint8 glyphSet);

};

inline ThaiShaping::StateTransition ThaiShaping::getTransition(le_uint8 state, le_uint8 currClass)
{
    return thaiStateTable[state][currClass];
}

U_NAMESPACE_END
#endif


