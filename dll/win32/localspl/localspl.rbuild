<module name="localspl" type="win32dll" baseaddress="${BASEADDRESS_LOCALSPL}" installbase="system32" installname="localspl.dll" allowwarnings="true">
	<importlibrary definition="localspl.spec" />
	<include base="localspl">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<file>localmon.c</file>
	<file>localspl_main.c</file>
	<file>provider.c</file>
	<file>localspl.rc</file>
	<library>wine</library>
	<library>spoolss</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
