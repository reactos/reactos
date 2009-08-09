<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="kbsdll" type="win32dll" baseaddress="0x74720000" installbase="system32" installname="kbsdll.dll" unicode="yes">
	<importlibrary definition="kbsdll.spec" />
	<include base="kbsdll">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<file>kbsdll.c</file>
	<file>kbsdll.rc</file>
</module>
