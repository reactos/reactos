<module name="devcpux" type="win32dll" installbase="system32" installname="devcpux.dll">
	<importlibrary definition="devcpux.def" />
	<include base="devcpux">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>ntdll</library>
	<library>powrprof</library>
	<library>comctl32</library>
	<file>processor.c</file>
	<file>processor.rc</file>
</module>
