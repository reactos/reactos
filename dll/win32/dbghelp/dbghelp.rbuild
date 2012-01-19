<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="dbghelp" type="win32dll" baseaddress="${BASEADDRESS_DBGHELP}" installbase="system32" installname="dbghelp.dll" allowwarnings="true" crt="msvcrt">
	<importlibrary definition="dbghelp.spec" />
	<include base="dbghelp">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WINE" />
	<define name="HAVE_REGEX_H" />
	<define name="_IMAGEHLP_SOURCE_" />
	<if property="ARCH" value="amd64">
		<define name="UNW_FLAG_NHANDLER">0</define>
		<define name="UNW_FLAG_EHANDLER">1</define>
		<define name="UNW_FLAG_UHANDLER">2</define>
		<define name="UNW_FLAG_CHAININFO">3</define>
	</if>
	<define name="lseek">_lseek</define>
    <define name="read">_read</define>
	<file>coff.c</file>
	<file>cpu_i386.c</file>
	<file>cpu_ppc.c</file>
	<file>cpu_x86_64.c</file>
	<file>crc32.c</file>
	<file>dbghelp.c</file>
	<file>dwarf.c</file>
	<file>elf_module.c</file>
	<file>image.c</file>
	<file>macho_module.c</file>
	<file>minidump.c</file>
	<file>module.c</file>
	<file>msc.c</file>
	<file>path.c</file>
	<file>pe_module.c</file>
	<file>regex.c</file>
	<file>rosstubs.c</file>
	<file>source.c</file>
	<file>stabs.c</file>
	<file>stack.c</file>
	<file>storage.c</file>
	<file>symbol.c</file>
	<file>type.c</file>
	<file>version.rc</file>
	<pch>dbghelp_private.h</pch>
	<library>wine</library>
	<library>psapi</library>
	<library>version</library>
	<library>ntdll</library>
	<library>pseh</library>
</module>
</group>
