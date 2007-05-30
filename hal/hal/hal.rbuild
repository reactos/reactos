<module name="hal" type="kernelmodedll">
	<importlibrary basename="hal" definition="hal.def" />
	<include base="ntoskrnl">include</include>
	<library>ntoskrnl</library>
	<define name="_NTHAL_" />
	<define name="__USE_W32API" />
    <linkerflag>-enable-stdcall-fixup</linkerflag>
	<file>hal.c</file>
	<file>hal.rc</file>
</module>

<module ifnot="${MP}" name="halupalias" type="alias" installbase="system32" installname="hal.dll" aliasof="halup">
</module>

<module if="${MP}" name="halmpalias" type="alias" installbase="system32" installname="hal.dll" aliasof="halmp">
</module>
