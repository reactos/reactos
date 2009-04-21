<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdda" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdda.dll">
	<importlibrary definition="kbdda.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdda.c</file>
	<file>kbdda.rc</file>
</module>
