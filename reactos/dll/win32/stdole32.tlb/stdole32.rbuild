<group>
<module name="std_ole_v1" type="embeddedtypelib">
	<include base="std_ole_v1">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<file>std_ole_v1.idl</file>
</module>
<module name="stdole32.tlb" type="win32dll" extension=".tlb" installbase="system32" installname="stdole32.tlb" entrypoint="0">
	<importlibrary definition="stdole32.tlb.spec.def" />
	<include base="stdole32.tlb">.</include>
	<include base="stdole32.tlb" root="intermediate">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<dependency>std_ole_v1</dependency>
	<define name="__WINESRC__" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>kernel32</library>
	<file>stdole32.tlb.spec</file>
	<file>rsrc.rc</file>
</module>
</group>