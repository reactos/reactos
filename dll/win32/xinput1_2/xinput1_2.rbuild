<module name="xinput1_2" type="win32dll" installbase="system32" installname="xinput1_2.dll">
	<importlibrary definition="xinput1_2.spec" />
	<include base="xinput1_2">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>kernel32</library>
	<file>xinput1_2_main.c</file>
	<file>version.rc</file>
</module>
