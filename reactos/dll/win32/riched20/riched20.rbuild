<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="riched20" type="win32dll" baseaddress="${BASEADDRESS_RICHED20}" installbase="system32" installname="riched20.dll" allowwarnings="true">
	<importlibrary definition="riched20.spec" />
	<include base="riched20">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<linkerflag>-enable-stdcall-fixup</linkerflag>
	<file>caret.c</file>
	<file>clipboard.c</file>
	<file>context.c</file>
	<file>editor.c</file>
	<file>list.c</file>
	<file>paint.c</file>
	<file>para.c</file>
	<file>reader.c</file>
	<file>richole.c</file>
	<file>row.c</file>
	<file>run.c</file>
	<file>string.c</file>
	<file>style.c</file>
	<file>table.c</file>
	<file>txtsrv.c</file>
	<file>undo.c</file>
	<file>wrap.c</file>
	<file>writer.c</file>
	<file>version.rc</file>
	<file>riched20.spec</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>imm32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
