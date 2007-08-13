<module name="shfolder" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_SHFOLDER}" installbase="system32" installname="shfolder.dll" allowwarnings="true">
	<importlibrary definition="shfolder.spec.def" />
	<include base="shfolder">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>shell32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>version.rc</file>
	<file>shfolder.spec</file>
</module>
