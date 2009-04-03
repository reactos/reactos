<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="libxslt" type="win32dll" entrypoint="0" installbase="system32" installname="libxslt.dll" allowwarnings="true" crt="msvcrt">
	<define name="HAVE_CONFIG_H" />
	<define name="WIN32" />
	<define name="_WINDOWS" />
	<define name="_MBCS" />
	<define name="HAVE_WIN32_THREADS" />
	<define name="_REENTRANT" />
	<define name="_WINSOCKAPI_" />
	<define name="LIBXML_STATIC" />
	<include base="libxslt">include</include>
	<include base="libxslt">.</include>
	<library>libxml2</library>
	<library>ws2_32</library>
	<file>attributes.c</file>
	<file>attrvt.c</file>
	<file>documents.c</file>
	<file>extensions.c</file>
	<file>extra.c</file>
	<file>functions.c</file>
	<file>imports.c</file>
	<file>keys.c</file>
	<file>namespaces.c</file>
	<file>numbers.c</file>
	<file>pattern.c</file>
	<file>preproc.c</file>
	<file>security.c</file>
	<file>templates.c</file>
	<file>transform.c</file>
	<file>variables.c</file>
	<file>xslt.c</file>
	<file>xsltutils.c</file>
</module>
