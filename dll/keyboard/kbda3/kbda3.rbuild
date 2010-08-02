<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbda3" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbda3.dll">
	<importlibrary definition="kbda3.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbda3.c</file>
	<file>kbda3.rc</file>
</module>
