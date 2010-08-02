<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdcan" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdcan.dll">
	<importlibrary definition="kbdcan.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdcan.c</file>
	<file>kbdcan.rc</file>
</module>
