<module name="compstui" type="win32dll" baseaddress="${BASEADDRESS_COMPSTUI}" installbase="system32" installname="compstui.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="compstui.spec.def" />
        <include base="compstui">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>compstui_main.c</file>
        <file>compstui.spec</file>
</module>
