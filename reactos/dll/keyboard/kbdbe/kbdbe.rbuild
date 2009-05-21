<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbe" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbe.dll">
	<importlibrary definition="kbdbe.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdbe.c</file>
	<file>kbdbe.rc</file>
</module>
