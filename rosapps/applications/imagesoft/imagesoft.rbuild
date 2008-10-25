<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="imagesoft" type="win32gui" installbase="system32" installname="imagesoft.exe" unicode="yes">
		<include base="imagesoft">.</include>
		<library>kernel32</library>
		<library>gdi32</library>
		<library>user32</library>
		<library>advapi32</library>
		<library>version</library>
		<library>comctl32</library>
		<library>shell32</library>
		<library>comdlg32</library>
		<compilationunit name="unit.c">
			<file>about.c</file>
			<file>adjust.c</file>
			<file>brightness.c</file>
			<file>contrast.c</file>
			<file>custcombo.c</file>
			<file>floatwindow.c</file>
			<file>font.c</file>
			<file>imagesoft.c</file>
			<file>imgedwnd.c</file>
			<file>mainwnd.c</file>
			<file>opensave.c</file>
			<file>tooldock.c</file>
			<file>misc.c</file>
		</compilationunit>
		<file>imagesoft.rc</file>
		<pch>precomp.h</pch>
	</module>
</rbuild>
