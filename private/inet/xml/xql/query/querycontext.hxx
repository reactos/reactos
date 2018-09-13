/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XQL_QUERY_CONTEXT
#define _XQL_QUERY_CONTEXT

#include "core/lang/object.hxx"

DEFINE_CLASS(QueryContext);
DEFINE_CLASS(Element);
DEFINE_CLASS(Query);

class NOVTABLE QueryContext // : public Object
{
public:
    virtual Element *   getDataElement(int idx) =0;
    virtual Query *     getQuery(int idx) =0;

protected:
	
private:
};


#endif
