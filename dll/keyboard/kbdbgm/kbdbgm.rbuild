<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbgm" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbgm.dll">
	<importlibrary definition="kbdbgm.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdbgm.c</file>
	<file>kbdbgm.rc</file>
</module>
