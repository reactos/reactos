#ifndef FILE_READER_H__
#define FILE_READER_H__

/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     pipe reader support
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "user_types.h"
#include "data_source.h"
#include <vector>
namespace System_
{
	using std::vector;
//---------------------------------------------------------------------------------------
///
/// class FileReader
///
/// Description: this class implements reading from a file

    class FileReader : public DataSource
	{
	public:
//---------------------------------------------------------------------------------------
///
/// FileReader
///
/// Description: constructor of class FileReader

	FileReader();

//---------------------------------------------------------------------------------------
///
/// ~FileReader
///
/// Description: destructor of class FileReader

	virtual ~FileReader();

//---------------------------------------------------------------------------------------
///
/// openFile
///
/// Description: attempts to open a file. Returns true on success
///
/// @param filename name of the file to open
/// @return bool

	virtual bool openSource(const string & filename);
//---------------------------------------------------------------------------------------
///
/// closeFile
///
/// Description: attempts to close a file. Returns true on success
///
/// @return bool

	virtual bool closeSource();

//---------------------------------------------------------------------------------------
///
/// readFile
///
/// Description: reads from file. The result is stored in a vector of strings
///
/// Note: returns true on success
///

	virtual bool readSource(vector<string> & lines);



	protected:
		FILE * m_File;
		string m_BufferedLines;


	}; // end of class FileReader


} // end of namespace System_


#endif /* end of FILE_READER_H__ */
