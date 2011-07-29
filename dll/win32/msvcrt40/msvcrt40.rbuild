<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvcrt40" type="win32dll" baseaddress="${BASEADDRESS_MSVCRT40}" installbase="system32" installname="msvcrt40.dll" iscrt="yes">
	<importlibrary definition="msvcrt40.spec" />
	<include base="msvcrt40">.</include>
	<include base="crt">include</include>
    <define name="USE_MSVCRT_PREFIX" />
	<define name="_MSVCRT_" />
	<define name="_MSVCRT_LIB_" />
	<define name="_MT" />
	<define name="_CTYPE_DISABLE_MACROS" />
	<define name="_NO_INLINING" />
	<define name="CRTDLL" />
	<file>msvcrt40.c</file>
    <file>stubs.c</file>
	<library>wine</library>
    <library>crt</library>
    <library>pseh</library>
</module>
</group>
