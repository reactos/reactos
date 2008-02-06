<module name="atl" type="win32dll" baseaddress="${BASEADDRESS_ATL}" installbase="system32" installname="atl.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="atl.spec.def" />
        <include base="atl">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__REACTOS__" />
        <define name="__WINESRC__" />
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x600</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>ole32</library>
        <library>oleaut32</library>
        <library>user32</library>
        <library>gdi32</library>
        <library>advapi32</library>
        <library>kernel32</library>
        <library>uuid</library>
        <library>ntdll</library>
        <file>atl_ax.c</file>
        <file>atl_main.c</file>
        <file>registrar.c</file>
        <file>rsrc.rc</file>
        <file>atliface.idl</file>
        <include base="atl" root="intermediate">.</include>
        <file>atl.spec</file>
</module>