#ifndef __RBUILD_H
#define __RBUILD_H

#include <string>
#include <vector>

using std::string;
using std::vector;

class XMLFile
{
	friend class XMLElement;
public:
	XMLFile();
	void close();
	bool open(const char* filename);
	void next_token();
	bool next_is_text();
	bool more_tokens();
	bool get_token(string& token);

private:
	vector<FILE*> _f;
	string _buf;

	const char *_p, *_end;
};


class XMLAttribute
{
public:
	string name;
	string value;

	XMLAttribute();
	XMLAttribute ( const string& name_, const string& value_ );
//		: name(name_), value(value_);
	XMLAttribute ( const XMLAttribute& src );
	XMLAttribute& operator = ( const XMLAttribute& src );
};


class XMLElement
{
public:
	string name;
	vector<XMLAttribute> attributes;
	vector<XMLElement*> subElements;
	string value;

	XMLElement();
	bool Parse(const string& token,
	           bool& end_tag);
};


class Module
{
public:
	Module(XMLElement moduleNode);
};

#endif /* __RBUILD_H */
