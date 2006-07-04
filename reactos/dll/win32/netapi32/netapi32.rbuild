<module name="netapi32" type="win32dll" baseaddress="${BASEADDRESS_NETAPI32}" installbase="system32" installname="netapi32.dll" allowwarnings="true">
	<importlibrary definition="netapi32.spec.def" />
	<include base="netapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>iphlpapi</library>
	<library>ws2_32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>access.c</file>
	<library>ntdll</library>
	<file>apibuf.c</file>
	<library>ntdll</library>
	<file>browsr.c</file>
	<library>ntdll</library>
	<file>ds.c</file>
	<library>ntdll</library>
	<file>nbcmdqueue.c</file>
	<library>ntdll</library>
	<file>nbnamecache.c</file>
	<library>ntdll</library>
	<file>nbt.c</file>
	<library>ntdll</library>
	<file>netapi32.c</file>
	<library>ntdll</library>
	<file>netbios.c</file>
	<library>ntdll</library>
	<file>wksta.c</file>
	<file>netapi32.spec</file>
</module>
