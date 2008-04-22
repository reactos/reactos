<module name="dbghelp" type="win32dll" baseaddress="${BASEADDRESS_DBGHELP}" installbase="system32" installname="dbghelp.dll" allowwarnings="true">
	<importlibrary definition="dbghelp.def" />
	<include base="dbghelp">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x502</define>
	<define name="WINVER">0x502</define>
	<define name="__WINESRC__" />
	<define name="HAVE_REGEX_H" />
	<library>ntdll</library>
	<library>wine</library>
	<library>pseh</library>
	<library>kernel32</library>
	<library>psapi</library>
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
	<file>stubs.c</file>
	<file>storage.c</file>
	<file>symbol.c</file>
	<file>type.c</file>
	<file>dbghelp.rc</file>
</module>
