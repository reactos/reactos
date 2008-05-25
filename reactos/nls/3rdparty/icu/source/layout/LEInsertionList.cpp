/*
 **********************************************************************
 *   Copyright (C) 1998-2004, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#include "LETypes.h"
#include "LEInsertionList.h"

U_NAMESPACE_BEGIN

#define ANY_NUMBER 1

struct InsertionRecord
{
    InsertionRecord *next;
    le_int32 position;
    le_int32 count;
    LEGlyphID glyphs[ANY_NUMBER];
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(LEInsertionList)

LEInsertionList::LEInsertionList(le_bool rightToLeft)
: head(NULL), tail(NULL), growAmount(0), append(rightToLeft)
{
    tail = (InsertionRecord *) &head;
}

LEInsertionList::~LEInsertionList()
{
    reset();
}

void LEInsertionList::reset()
{
    while (head != NULL) {
        InsertionRecord *record = head;

        head = head->next;
        LE_DELETE_ARRAY(record);
    }

    tail = (InsertionRecord *) &head;
    growAmount = 0;
}

le_int32 LEInsertionList::getGrowAmount()
{
    return growAmount;
}

LEGlyphID *LEInsertionList::insert(le_int32 position, le_int32 count)
{
    InsertionRecord *insertion = (InsertionRecord *) LE_NEW_ARRAY(char, sizeof(InsertionRecord) + (count - ANY_NUMBER) * sizeof (LEGlyphID));

    insertion->position = position;
    insertion->count = count;

    growAmount += count - 1;

    if (append) {
        // insert on end of list...
        insertion->next = NULL;
        tail->next = insertion;
        tail = insertion;
    } else {
        // insert on front of list...
        insertion->next = head;
        head = insertion;
    }

    return insertion->glyphs;
}

le_bool LEInsertionList::applyInsertions(LEInsertionCallback *callback)
{
    for (InsertionRecord *rec = head; rec != NULL; rec = rec->next) {
        if (callback->applyInsertion(rec->position, rec->count, rec->glyphs)) {
            return TRUE;
        }
    }

    return FALSE;
}

U_NAMESPACE_END
