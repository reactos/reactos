/*
 * @(#)Entity.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "entity.hxx"
#include "xmlnames.hxx"

DEFINE_CLASS_MEMBERS_CLASS(Entity, _T("Entity"), Base);

/**
 * Entity object representing an XML internal/external entity.
 *
 * @version 1.0, 6/3/97
 */

Entity::Entity(Name * name, bool par)
{
    this->name = name;
    this->par = par;
    this->validating = false;
}

Entity::Entity(Name * name, bool par, String * text)
{
    this->name = name;
    this->par = par;
    this->validating = false;
    setText(text);
    setPosition(0, 0);
}

Entity::Entity(Name * name, bool par, int c)
{
    this->name = name;
    this->par = par;
    this->validating = false;
    cdata = (TCHAR)c;
    setText(String::valueOf(cdata));
    setPosition(0, 0);
}

void Entity::setURL(String * url)
{
    this->url = url;
    sys = true;
}

/**
 * Changes the text of entity.
 * @param text  The new text of the entity.
 */
void Entity::setText(String * text)
{
    this->text = text;
    sys = false;
}

void Entity::setPosition(ULONG line, ULONG column)
{
    this->line = line;
    this->column = column;
}

int Entity::getLength()
{
    if (cdata > 0)
        return -1;
    else if (text == null)
        return 0;
    else return text->length();
}

TCHAR Entity::getChar(int index)
{
    if (text == null)
        return cdata;
    else
        return text->charAt(index);
}
