<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth3" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdth3.dll">
	<importlibrary definition="kbdth3.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdth3.c</file>
	<file>kbdth3.rc</file>
</module>
