<module name="dciman32" type="win32dll" baseaddress="${BASEADDRESS_DCIMAN32}" installbase="system32" installname="dciman32.dll" unicode="yes">
	<importlibrary definition="dciman32.def" />
	<include base="dciman32">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>ntdll</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>pseh</library>
	<library>dxguid</library>
	<library>gdi32</library>
	<pch>precomp.h</pch>
	<file>dciman32_main.c</file>

</module>

