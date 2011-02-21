<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wer" type="win32dll" installbase="system32" installname="wer.dll" unicode="yes">
	<importlibrary definition="wer.spec" />
	<include base="wer">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>advapi32</library>
	<library>wine</library>
	<file>main.c</file>
</module>
