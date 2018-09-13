/* @(#)ElementStream.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of ElementStream object
 * 
 */


#ifndef _XTL_OUTPUT_ELEMENTSTREAM
#define _XTL_OUTPUT_ELEMENTSTREAM

#ifndef _XTL_ENGINE_XTLPROCESSOREVENTS
#include "xtl/engine/xtlprocessorevents.hxx"
#endif

DEFINE_CLASS(ElementStream);

/**
 * An action that contains other actions and establishes a lexical scope for template lookup.
 *
 * Hungarian: xtl
 *
 */

class ElementStream : public Base, public XTLProcessorEvents
{

    DECLARE_CLASS_MEMBERS_I1(ElementStream, Base, XTLProcessorEvents);

    public:

        static ElementStream * newElementStream(OutputHelper * out);

        void beginElement(int type, String * name, String * text, bool fNoEntities); 
        void beginChildren(int parentType, bool hasWSInside);   
        void endElement(int type, String * text, bool hasChildren, bool hasWSAfter); 
        void close();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        ElementStream(OutputHelper * out);

        virtual void finalize();

         // hide these (not implemented)

        ElementStream(){}
        ElementStream( const ElementStream &);
        void operator =( const ElementStream &);


    private:

        ROutputHelper   _out;
        int             _fDontExpandEntities;        
        int             _fIsAttr;        
};


#endif _XTL_OUTPUT_ELEMENTSTREAM

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////