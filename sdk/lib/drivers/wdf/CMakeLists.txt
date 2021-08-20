
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE_SHARED
    shared/core/fxcxdeviceinit.cpp
    # shared/core/fxcxdeviceinitapi.cpp
    shared/core/fxdevice.cpp
    shared/core/fxdeviceapi.cpp
    shared/core/fxdevicebase.cpp
    shared/core/fxdevicecontrolapi.cpp
    shared/core/fxdeviceinit.cpp
    shared/core/fxdeviceinitapi.cpp
    shared/core/fxdisposelist.cpp
    shared/core/fxdriver.cpp
    shared/core/fxdriverapi.cpp
    shared/core/fxfileobject.cpp
    shared/core/fxfileobjectapi.cpp
    shared/core/fxirpqueue.cpp
    # shared/core/fxlock.cpp
    shared/core/fxlookasidelist.cpp
    shared/core/fxlookasidelistapi.cpp
    shared/core/fxmemorybuffer.cpp
    shared/core/fxmemorybufferapi.cpp
    shared/core/fxmemorybufferfromlookaside.cpp
    shared/core/fxmemorybufferfrompool.cpp
    shared/core/fxmemorybufferpreallocated.cpp
    shared/core/fxmemorybufferpreallocatedapi.cpp
    shared/core/fxmemoryobject.cpp
    shared/core/fxpagedlookasidelist.cpp
    shared/core/fxrequest.cpp
    shared/core/fxrequestapi.cpp
    shared/core/fxrequestbase.cpp
    shared/core/fxrequestcontext.cpp
    shared/core/fxrequestmemory.cpp
    shared/core/fxrequestoutputbuffer.cpp
    shared/core/fxrequestsystembuffer.cpp
    shared/core/fxsyncrequest.cpp
    shared/core/fxsystemworkitem.cpp
    shared/core/fxtimer.cpp
    shared/core/fxtimerapi.cpp
    shared/core/fxworkitem.cpp
    shared/core/fxworkitemapi.cpp
    # shared/core/verifierapi.cpp
    shared/irphandlers/default/fxdefaultirphandler.cpp
    shared/irphandlers/general/fxpkggeneral.cpp
    shared/irphandlers/io/fxioqueue.cpp
    shared/irphandlers/io/fxioqueueapi.cpp
    shared/irphandlers/io/fxpkgio.cpp
    shared/irphandlers/package/fxpackage.cpp
    # shared/irphandlers/pnp/devicepwrreqstatemachine.cpp
    shared/irphandlers/pnp/eventqueue.cpp
    shared/irphandlers/pnp/fdopower.cpp
    shared/irphandlers/pnp/fxinterruptapi.cpp
    shared/irphandlers/pnp/fxpkgfdo.cpp
    shared/irphandlers/pnp/fxpkgpdo.cpp
    shared/irphandlers/pnp/fxpkgpnp.cpp
    shared/irphandlers/pnp/interruptobject.cpp
    shared/irphandlers/pnp/notpowerpolicyownerstatemachine.cpp
    shared/irphandlers/pnp/pdopower.cpp
    shared/irphandlers/pnp/pnpstatemachine.cpp
    shared/irphandlers/pnp/poweridlestatemachine.cpp
    shared/irphandlers/pnp/powerpolicystatemachine.cpp
    shared/irphandlers/pnp/powerstatemachine.cpp
    shared/irphandlers/pnp/poxinterface.cpp # stubs
    shared/irphandlers/pnp/selfmanagediostatemachine.cpp
    shared/irphandlers/pnp/wakeinterruptstatemachine.cpp
    shared/object/dbgtrace.cpp
    shared/object/fxobject.cpp
    shared/object/fxobjectapi.cpp
    shared/object/fxobjectstatemachine.cpp
    shared/object/fxtagtracker.cpp
    shared/object/fxuserobject.cpp
    shared/object/fxuserobjectapi.cpp
    shared/object/fxvalidatefunctions.cpp
    shared/object/fxverifierbugcheck.cpp
    shared/object/fxverifierlock.cpp
    shared/object/globals.cpp
    shared/object/handleapi.cpp
    shared/object/wdfpool.cpp
    shared/support/fxcollection.cpp
    shared/support/fxcollectionapi.cpp
    # shared/support/fxdeviceinterface.cpp
    shared/support/fxdeviceinterfaceapi.cpp
    shared/support/fxdevicetext.cpp
    shared/support/fxregistryapi.cpp
    shared/support/fxregkey.cpp
    shared/support/fxrequestbuffer.cpp
    shared/support/fxresourceapi.cpp
    shared/support/fxresourcecollection.cpp
    shared/support/fxspinlock.cpp
    shared/support/fxspinlockapi.cpp
    shared/support/fxstring.cpp
    shared/support/fxstringapi.cpp
    # shared/support/fxtelemetry.cpp
    shared/support/fxtransactionedlist.cpp
    shared/support/fxwaitlock.cpp
    shared/support/fxwaitlockapi.cpp
    shared/support/stringutil.cpp
    shared/targets/general/fxiotarget.cpp
    shared/targets/general/fxiotargetapi.cpp
    shared/targets/general/fxiotargetremote.cpp
    shared/targets/general/fxiotargetself.cpp
    # shared/targets/usb/fxusbdevice.cpp
    # shared/targets/usb/fxusbdeviceapi.cpp
    # shared/targets/usb/fxusbinterface.cpp
    # shared/targets/usb/fxusbinterfaceapi.cpp
    # shared/targets/usb/fxusbpipe.cpp
    # shared/targets/usb/fxusbpipeapi.cpp
    # shared/targets/usb/usbutil.cpp
    )

list(APPEND SOURCE_KM
    kmdf/src/core/fxbugcheckcallback.cpp
    kmdf/src/core/fxchildlist.cpp
    kmdf/src/core/fxchildlistapi.cpp
    kmdf/src/core/fxdevicefdoapi.cpp
    kmdf/src/core/fxdevicepdoapi.cpp
    kmdf/src/core/fxdpc.cpp
    kmdf/src/core/fxdpcapi.cpp
    kmdf/src/core/fxsystemthread.cpp
    # kmdf/src/core/tracing.cpp
    # kmdf/src/dma/base/fxcommonbuffer.cpp # requires DMA stuff
    # kmdf/src/dma/base/fxcommonbufferapi.cpp # requires DMA stuff
    # kmdf/src/dma/base/fxdmaenabler.cpp
    # kmdf/src/dma/base/fxdmaenablerapi.cpp
    # kmdf/src/dma/base/fxdmatransaction.cpp
    # kmdf/src/dma/base/fxdmatransactionapi.cpp
    # kmdf/src/dynamic/version/version.cpp
    kmdf/src/fxtosharedinterface/fxobject/fxobjectinfokm.cpp
    # kmdf/src/irphandlers/wmi/fxwmiapi.cpp
    # kmdf/src/irphandlers/wmi/fxwmiinstance.cpp
    # kmdf/src/irphandlers/wmi/fxwmiirphandler.cpp
    # kmdf/src/irphandlers/wmi/fxwmiprovider.cpp
    # kmdf/src/librarycommon/fxlibrarycommon.cpp
    kmdf/src/support/fxqueryinterface.cpp
    kmdf/src/support/fxqueryinterfaceapi.cpp
    kmdf/src/support/fxrelateddevice.cpp
    kmdf/src/support/fxrelateddevicelist.cpp
    kmdf/src/support/probeandlock.c
    shared/core/km/fxdeviceapikm.cpp
    shared/core/km/fxdevicebasekm.cpp
    shared/core/km/fxdeviceinitkm.cpp
    shared/core/km/fxdevicekm.cpp
    shared/core/km/fxdriverapikm.cpp
    shared/core/km/fxdriverkm.cpp
    shared/core/km/fxfileobjectapikm.cpp
    shared/core/km/fxfileobjectkm.cpp
    shared/core/km/fxnpagedlookasidelistkm.cpp
    shared/core/km/fxrequestkm.cpp
    shared/core/km/fxrequestsystembufferkm.cpp
    shared/irphandlers/io/km/fxioqueuekm.cpp
    shared/irphandlers/pnp/km/eventqueuekm.cpp
    shared/irphandlers/pnp/km/fxpkgfdokm.cpp
    shared/irphandlers/pnp/km/fxpkgpdokm.cpp
    shared/irphandlers/pnp/km/fxpkgpnpkm.cpp
    shared/irphandlers/pnp/km/interruptobjectkm.cpp
    shared/irphandlers/pnp/km/pnpstatemachinekm.cpp
    shared/irphandlers/pnp/km/powerpolicystatemachinekm.cpp
    shared/irphandlers/pnp/km/powerstatemachinekm.cpp # stubs
    # shared/irphandlers/pnp/km/poxinterfacekm.cpp
    shared/irphandlers/pnp/km/supportkm.cpp
    shared/object/km/fxobjectkm.cpp
    shared/object/km/globalskm.cpp
    shared/object/km/wdfpoolkm.cpp
    shared/primitives/km/mxgeneralkm.cpp
    shared/support/km/fxdeviceinterfacekm.cpp
    shared/support/km/fxregkeykm.cpp
    shared/support/km/fxrequestbufferkm.cpp
    shared/support/km/fxresourcecollectionkm.cpp
    # shared/support/km/fxtelemetrykm.cpp
    shared/targets/general/fxiotarget.cpp
    shared/targets/general/fxiotargetapi.cpp
    shared/targets/general/fxiotargetremote.cpp
    shared/targets/general/fxiotargetself.cpp
    shared/targets/general/km/fxiotargetapikm.cpp
    shared/targets/general/km/fxiotargetkm.cpp
    shared/targets/general/km/fxiotargetremotekm.cpp
    reactos_special.cpp
    )

# add_library(wdf01000 MODULE ${SOURCE})

# set_module_type(wdf01000 kernelmodedriver ENTRYPOINT 0)
# add_importlibs(wdf01000 ntoskrnl hal wdfldr)
# add_cd_file(TARGET wdf01000 DESTINATION reactos/system32/drivers FOR all)
# add_registry_inf(wdf01000_reg.inf)

add_library(wdf01000 ${SOURCE_SHARED} ${SOURCE_KM})

target_compile_definitions(wdf01000
    PUBLIC WDF_DEVICE_NO_WDMSEC_H _WIN32_WINNT=0x603 NTDDI_VERSION=0x06030000 # NTDDI_WINBLUE
    PRIVATE FX_CORE_MODE=1 FX_CORE_KERNEL_MODE=1
    PRIVATE __WDF_MAJOR_VERSION=1 __WDF_MINOR_VERSION=17 __WDF_BUILD_NUMBER=7600)

target_include_directories(wdf01000
    PUBLIC ${REACTOS_SOURCE_DIR}/sdk/include/wdf/kmdf/1.17
    PRIVATE
        shared/inc/primitives/common
        shared/inc/primitives/km
        shared/inc/private/common
        shared/inc/private/km
        kmdf/inc/private
        shared/object # fxobjectpch.hpp
        shared/support # fxsupportpch.hpp
        shared/core # coreprivshared.hpp
        shared/irphandlers # irphandlerspriv.hpp
        shared/irphandlers/pnp # pnppriv.hpp
        shared/irphandlers/pnp/km # pnpprivkm.hpp
    )

target_link_libraries(wdf01000 aux_klib ntoskrnl_vista ${PSEH_LIB})

if(GCC)
    target_compile_options(wdf01000
        PRIVATE -Wno-write-strings -Wno-unknown-pragmas -Wno-switch
        PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder -Wno-invalid-offsetof -Wno-delete-non-virtual-dtor>)
endif()
