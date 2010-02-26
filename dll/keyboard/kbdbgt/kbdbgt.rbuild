<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbgt" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbgt.dll">
	<importlibrary definition="kbdbgt.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdbgt.c</file>
	<file>kbdbgt.rc</file>
</module>
