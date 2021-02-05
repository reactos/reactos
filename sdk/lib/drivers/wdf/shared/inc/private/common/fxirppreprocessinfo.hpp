//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXIRPPREPROCESSINFO_H_
#define _FXIRPPREPROCESSINFO_H_

struct FxIrpPreprocessInfo : public FxStump {
    FxIrpPreprocessInfo() :
        ClassExtension(FALSE)
    {
        InitializeListHead(&ListEntry);
    }

    ~FxIrpPreprocessInfo()
    {
        ASSERT(IsListEmpty(&ListEntry));
    }

    struct Info {
        Info() :
            EvtDevicePreprocess(NULL),
            NumMinorFunctions(0),
            MinorFunctions(NULL)
        {
        }

        ~Info()
        {
            if (MinorFunctions != NULL) {
                FxPoolFree(MinorFunctions);
            }
        }

        union {
            PFN_WDFDEVICE_WDM_IRP_PREPROCESS        EvtDevicePreprocess;
            PFN_WDFCXDEVICE_WDM_IRP_PREPROCESS      EvtCxDevicePreprocess;
        };

        ULONG       NumMinorFunctions;
        PUCHAR      MinorFunctions;
    };

    LIST_ENTRY  ListEntry;
    Info        Dispatch[IRP_MJ_MAXIMUM_FUNCTION+1];
    BOOLEAN     ClassExtension;
};


#endif // _FXIRPPREPROCESSINFO_H_
