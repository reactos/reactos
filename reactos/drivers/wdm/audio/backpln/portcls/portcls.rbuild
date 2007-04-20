<module name="portcls" type="kernelmodedriver" installbase="system32/drivers" installname="portcls.sys" allowwarnings="true">
    <linkerflag>-fno-exceptions</linkerflag>
    <linkerflag>-fno-rtti</linkerflag>
        <importlibrary definition="portcls.def" />
    <define name="__USE_W32API" />
    <define name="_NTDDK_" />
    <include base="portcls">../include</include>
    <library>ntoskrnl</library>
    <library>ks</library>
    <library>drmk</library>

    <file>dll.c</file>
    <file>adapter.c</file>
    <file>irp.c</file>
    <file>drm.c</file>
    <file>stubs.c</file>

    <!-- Probably not the best idea to have this separate -->
    <!--<file>../stdunk/stdunk.c</file>-->

    <file>helper/ResourceList.c</file>

    <file>port/factory.c</file>

    <file>portcls.rc</file>
</module>
