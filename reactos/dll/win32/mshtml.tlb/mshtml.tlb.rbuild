<group>
<module name="mshtml_tlb" type="embeddedtypelib">
	<dependency>stdole2</dependency>
	<file>mshtml_tlb.idl</file>
</module>
<module name="mshtml.tlb" type="win32dll" extension=".tlb" installbase="system32" installname="mshtml.tlb" entrypoint="0">
	<importlibrary definition="mshtml.tlb.spec" />
	<include base="mshtml.tlb" root="intermediate">.</include>
	<dependency>mshtml_tlb</dependency>
	<define name="__WINESRC__" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>kernel32</library>
	<file>mshtml.tlb.spec</file>
	<file>rsrc.rc</file>
</module>
</group>