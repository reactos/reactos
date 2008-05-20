<?xml version="1.0"?>
<group xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="kbswitch" type="win32gui" installbase="system32" installname="kbswitch.exe" unicode="yes">
	<include base="kbswitch">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>shell32</library>
	<library>gdi32</library>
	<file>kbswitch.c</file>
	<file>kbswitch.rc</file>
</module>
<directory name="kbsdll">
	<xi:include href="kbsdll/kbsdll.rbuild" />
</directory>
</group>
