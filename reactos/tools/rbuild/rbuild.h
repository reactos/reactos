#ifndef __RBUILD_H
#define __RBUILD_H

#include <string>
#include <vector>

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
	bool get_token(std::string& token);

private:
	std::vector<FILE*> _f;
	std::string _buf;

	const char *_p, *_end;
};


class XMLAttribute
{
public:
	std::string name;
	std::string value;

	XMLAttribute();
	XMLAttribute ( const std::string& name_, const std::string& value_ );
};


class XMLElement
{
public:
	std::string name;
	std::vector<XMLAttribute*> attributes;
	std::vector<XMLElement*> subElements;
	std::string value;

	XMLElement();
	~XMLElement();
	bool Parse(const std::string& token,
	           bool& end_tag);
	const XMLAttribute* GetAttribute ( const std::string& attribute ) const;
};

class Project;
class Module;

class Project
{
public:
	std::string name;
	std::vector<Module*> modules;

	void ProcessXML ( const XMLElement& e, const std::string& path );
};

class Module
{
public:
	const XMLElement& node;
	std::string name;
	std::string path;

	Module ( const XMLElement& moduleNode,
	         const std::string& moduleName,
	         const std::string& modulePath );
};

#endif /* __RBUILD_H */
