#ifndef DATA_SOURCE_H__
#define DATA_SOURCE_H__


/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     data source abstraction
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#include <vector>
#include "user_types.h"

namespace System_
{


class DataSource
{
public:

    DataSource()
    {}

    virtual ~DataSource()
    {}

    virtual bool open(const string & opencmd) = 0;

    virtual bool close() = 0;

    virtual bool read(std::vector<string> & vect) = 0;

}; // end of class DataSource



} // end of namespace System_







#endif /* end of DATA_SOURCE_H__ */
