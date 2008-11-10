<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="dbghelp" type="win32dll" baseaddress="${BASEADDRESS_DBGHELP}" installbase="system32" installname="dbghelp.dll" allowwarnings="true">
	<importlibrary definition="dbghelp.spec" />
	<include base="dbghelp">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="HAVE_REGEX_H" />
	<file>coff.c</file>
	<file>dbghelp.c</file>
	<file>dwarf.c</file>
	<file>elf_module.c</file>
	<file>image.c</file>
	<file>memory.c</file>
	<file>minidump.c</file>
	<file>module.c</file>
	<file>msc.c</file>
	<file>path.c</file>
	<file>pe_module.c</file>
	<file>regex.c</file>
	<file>source.c</file>
	<file>stabs.c</file>
	<file>stack.c</file>
	<file>storage.c</file>
	<file>symbol.c</file>
	<file>type.c</file>
	<library>wine</library>
	<library>psapi</library>
	<library>kernel32</library>
	<library>version</library>
	<library>ntdll</library>
</module>
</group>
