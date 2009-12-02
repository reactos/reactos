<module name="msftedit" type="win32dll" baseaddress="${BASEADDRESS_MSFTEDIT}" installbase="system32" installname="msftedit.dll" allowwarnings="true">
	<importlibrary definition="msftedit.spec" />
	<include base="msftedit">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msftedit_main.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>riched20</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
