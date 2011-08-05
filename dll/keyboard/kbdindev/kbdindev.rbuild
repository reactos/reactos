<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdindev" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdindev.dll">
	<importlibrary definition="kbdindev.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdindev.c</file>
	<file>kbdindev.rc</file>
</module>
