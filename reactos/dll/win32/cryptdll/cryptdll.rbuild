<module name="cryptdll" type="win32dll" baseaddress="${BASEADDRESS_CRYPTDLL}" installbase="system32" installname="cryptdll.dll">
	<importlibrary definition="cryptdll.spec.def" />
	<include base="cryptdll">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x501</define>
	<define name="DLL_WINE_PREATTACH">8</define>
	<file>cryptdll.c</file>
	<file>cryptdll.spec</file>
</module>
