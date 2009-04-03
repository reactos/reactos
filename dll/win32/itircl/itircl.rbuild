<module name="itircl" type="win32dll" baseaddress="${BASEADDRESS_ITIRCL}" installbase="system32" installname="itircl.dll" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="DllRegisterServer" /-->
	<importlibrary definition="itircl.spec" />
	<include base="itircl">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>itircl_main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
