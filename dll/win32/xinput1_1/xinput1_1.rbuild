<module name="xinput1_1" type="win32dll" installbase="system32" installname="xinput1_1.dll">
	<importlibrary definition="xinput1_1.spec" />
	<include base="xinput1_1">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>kernel32</library>
	<file>xinput1_1_main.c</file>
	<file>version.rc</file>
</module>
