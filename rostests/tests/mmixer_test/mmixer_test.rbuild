<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mmixer_test" type="win32cui" baseaddress="${BASEADDRESS_CONTROL}" installbase="system32" installname="mmixer_test.exe">
	<include base="ReactOS">include/reactos/libs/sound</include>
	<include base="mmixer"></include>
	<library>advapi32</library>
	<library>setupapi</library>
	<library>kernel32</library>
	<library>winmm</library>
	<library>mmixer</library>
	<file>test.c</file>
</module>