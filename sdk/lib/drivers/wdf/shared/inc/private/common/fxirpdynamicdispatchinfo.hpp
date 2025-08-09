//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXIRPDYNAMICDISPATCHINFO_H_
#define _FXIRPDYNAMICDISPATCHINFO_H_

//
// Placeholder macro for a no-op
//
#ifndef DO_NOTHING
#define DO_NOTHING()                            (0)
#endif

struct FxIrpDynamicDispatchInfo : public FxStump {
    FxIrpDynamicDispatchInfo() :
        CxDeviceInfo(NULL)
    {
        InitializeListHead(&ListEntry);
        /* FIXME: Using RtlZeroMemory on non-trivial type generates -Wclass-memaccess warning
         * Info struct has a constructor, so memset is technically incorrect
         * The proper fix would be to use value initialization: Dispatch{} in initializer list
         * Using pragma to suppress -Wclass-memaccess warning */
#ifdef __GNUC__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
        RtlZeroMemory(Dispatch, sizeof(Dispatch));
#ifdef __GNUC__
        #pragma GCC diagnostic pop
#endif
    }

    ~FxIrpDynamicDispatchInfo()
    {
        ASSERT(IsListEmpty(&ListEntry));
    }

    enum DynamicDispatchType {
        DynamicDispatchRead            = 0,
        DynamicDispatchWrite           = 1,
        DynamicDispatchIoctl           = 2,
        DynamicDispatchInternalIoctl   = 3,
        DynamicDispatchMax
    };

    struct Info {
        Info() :
            EvtDeviceDynamicDispatch(NULL),
            DriverContext(NULL)
        {
        }

        ~Info()
        {
            DO_NOTHING();
        }

        PFN_WDFDEVICE_WDM_IRP_DISPATCH  EvtDeviceDynamicDispatch;
        WDFCONTEXT                      DriverContext;
    };

    __inline
    static
    int
    Mj2Index(
        UCHAR MajorFunction
        )
    {
        DynamicDispatchType type;

        switch (MajorFunction) {
        case IRP_MJ_READ:
            type = DynamicDispatchRead;
            break;

        case IRP_MJ_WRITE:
            type = DynamicDispatchWrite;
            break;

        case IRP_MJ_DEVICE_CONTROL:
            type = DynamicDispatchIoctl;
            break;

        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            type = DynamicDispatchInternalIoctl;
            break;

        default:
            type = DynamicDispatchMax;
            break;
        }

        return (int)type;
    }

    LIST_ENTRY          ListEntry;
    Info                Dispatch[DynamicDispatchMax];

    //
    // If not null, weak ref to class extension info struct.
    //
    FxCxDeviceInfo*     CxDeviceInfo;
};

#endif // _FXIRPDYNAMICDISPATCHINFO_H_
