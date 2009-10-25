<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="setupapi" type="win32dll" baseaddress="${BASEADDRESS_SETUPAPI}" installbase="system32" installname="setupapi.dll" allowwarnings="true" unicode="yes" crt="msvcrt">
	<importlibrary definition="setupapi.spec" />
	<include base="setupapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="pnp_client">.</include>
	<define name="_SETUPAPI_" />
	<library>pnp_client</library>
	<library>uuid</library>
	<library>wine</library>
	<library>ntdll</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>rpcrt4</library>
	<library>version</library>
	<library>shell32</library>
	<library>pseh</library>
	<file>cfgmgr.c</file>
	<file>devclass.c</file>
	<file>devinst.c</file>
	<file>dirid.c</file>
	<file>diskspace.c</file>
	<file>driver.c</file>
	<file>install.c</file>
	<file>interface.c</file>
	<file>misc.c</file>
	<file>parser.c</file>
	<file>query.c</file>
	<file>queue.c</file>
	<file>setupcab.c</file>
	<file>stringtable.c</file>
	<file>stubs.c</file>
	<file>rpc.c</file>
	<file>setupapi.rc</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>
