<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdro" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdro.dll">
	<importlibrary definition="kbdro.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdro.c</file>
	<file>kbdro.rc</file>
</module>
