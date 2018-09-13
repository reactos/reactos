/* @(#)XTLProcessorEvents.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of Processor events
 * 
 */


#ifndef _XTL_ENGINE_XTLPROCESSOREVENTS
#define _XTL_ENGINE_XTLPROCESSOREVENTS

/**
 * An action that contains other actions and establishes a lexical scope for template lookup.
 *
 * Hungarian: xtlevents
 *
 */

DEFINE_CLASS(XTLProcessorEvents);

class NOVTABLE XTLProcessorEvents : public Object
{
public:
    virtual void beginElement(int type, String * name, String * text, bool fNoEntities) = 0; 
    virtual void beginChildren(int parentType, bool hasWSInside) = 0;   
    virtual void endElement(int type, String * text, bool hasChildren, bool hasWSAfter) = 0; 
    virtual void close() = 0;
};


#endif _XTL_ENGINE_XTLPROCESSOREVENTS

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////