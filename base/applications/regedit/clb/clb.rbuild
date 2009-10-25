<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="clb" type="win32dll" baseaddress="${BASEADDRESS_CLB}" installbase="system32" installname="clb.dll" unicode="yes">
	<importlibrary definition="clb.spec" />
	<include base="clb">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<file>clb.c</file>
	<file>clb.rc</file>
	<pch>precomp.h</pch>
</module>
