<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
  <module name="msvcrt20" type="win32dll"
  baseaddress="${BASEADDRESS_MSVCRT20}" installbase="system32"
  installname="msvcrt20.dll"
  entrypoint="DllMain@12" iscrt="yes">
    <importlibrary definition="msvcrt20.spec" />
    <include base="msvcrt20">.</include>
    <include base="crt">include</include>
    <define name="USE_MSVCRT_PREFIX" />
	<define name="_MSVCRT_" />
	<define name="_MSVCRT_LIB_" />
	<define name="_MT" />
	<define name="_CTYPE_DISABLE_MACROS" />
	<define name="_NO_INLINING" />
	<define name="CRTDLL" />
    <file>msvcrt20.c</file>
    <file>stubs.c</file>
    <library>wine</library>
    <library>crt</library>
    <library>pseh</library>
  </module>
</group>
