<module name="bcrypt" type="win32dll" installbase="system32" installname="bcrypt.dll" allowwarnings="true">
	<importlibrary definition="bcrypt.spec" />
	<include base="bcrypt">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<file>bcrypt_main.c</file>
	<file>version.rc</file>
</module>
