<module name="winhttp" type="win32dll" baseaddress="${BASEADDRESS_WINHTTP}" installbase="system32" installname="winhttp.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="winhttp.spec.def" />
        <include base="winhttp">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__REACTOS__" />
        <define name="__WINESRC__" />
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>main.c</file>
        <file>winhttp.spec</file>
</module>