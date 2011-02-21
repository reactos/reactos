<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgerg" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdgerg.dll">
	<importlibrary definition="kbdgerg.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdgerg.c</file>
	<file>kbdgerg.rc</file>
</module>
