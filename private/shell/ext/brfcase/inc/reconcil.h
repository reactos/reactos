/*
 * reconcil.h - OLE reconciliation interface definitions.
 */


#ifndef __RECONCIL_H__
#define __RECONCIL_H__


/* Headers
 **********/

#include <recguids.h>


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Constants
 ************/

/* for use in IStorage::SetStateBits() */

#define STATEBITS_FLAT                 (0x0001)

/* reconciliation SCODEs */

#define REC_S_IDIDTHEUPDATES           MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x1000)
#define REC_S_NOTCOMPLETE              MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x1001)
#define REC_S_NOTCOMPLETEBUTPROPAGATE  MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x1002)

#define REC_E_ABORTED                  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x1000)
#define REC_E_NOCALLBACK               MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x1001)
#define REC_E_NORESIDUES               MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x1002)
#define REC_E_TOODIFFERENT             MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x1003)
#define REC_E_INEEDTODOTHEUPDATES      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x1004)


/* Interfaces
 *************/

#undef  INTERFACE
#define INTERFACE INotifyReplica

DECLARE_INTERFACE_(INotifyReplica, IUnknown)
{
   /* IUnknown methods */

   STDMETHOD(QueryInterface)(THIS_
                             REFIID riid,
                             PVOID *ppvObject) PURE;

   STDMETHOD_(ULONG, AddRef)(THIS) PURE;

   STDMETHOD_(ULONG, Release)(THIS) PURE;

   /* INotifyReplica methods */

   STDMETHOD(YouAreAReplica)(THIS_
                             ULONG ulcOtherReplicas,
                             IMoniker **rgpmkOtherReplicas) PURE;
};

#undef  INTERFACE
#define INTERFACE IReconcileInitiator

DECLARE_INTERFACE_(IReconcileInitiator, IUnknown)
{
   /* IUnknown methods */

   STDMETHOD(QueryInterface)(THIS_
                             REFIID riid,
                             PVOID *ppvObject) PURE;

   STDMETHOD_(ULONG, AddRef)(THIS) PURE;

   STDMETHOD_(ULONG, Release)(THIS) PURE;

   /* IReconcileInitiator methods */

   STDMETHOD(SetAbortCallback)(THIS_
                               IUnknown *punkForAbort) PURE;

   STDMETHOD(SetProgressFeedback)(THIS_
                                  ULONG ulProgress,
                                  ULONG ulProgressMax) PURE;
};

/* IReconcilableObject::Reconcile() flags */

typedef enum _reconcilef
{
   /* interaction with the user is allowed */

   RECONCILEF_MAYBOTHERUSER         = 0x0001,

   /*
    * hwndProgressFeedback may be used to provide reconciliation progress
    * feedback to the user.
    */

   RECONCILEF_FEEDBACKWINDOWVALID   = 0x0002,

   /* residue support not required */

   RECONCILEF_NORESIDUESOK          = 0x0004,

   /* caller not interested in callee's residues */

   RECONCILEF_OMITSELFRESIDUE       = 0x0008,

   /*
    * Reconcile() call resuming after a previous Reconcile() call returned
    * REC_E_NOTCOMPLETE
    */

   RECONCILEF_RESUMERECONCILIATION  = 0x0010,

   /* Object may perform all updates. */

   RECONCILEF_YOUMAYDOTHEUPDATES    = 0x0020,

   /* Only this object has been changed. */

   RECONCILEF_ONLYYOUWERECHANGED    = 0x0040,

   /* flag combinations */

   ALL_RECONCILE_FLAGS              = (RECONCILEF_MAYBOTHERUSER |
                                       RECONCILEF_FEEDBACKWINDOWVALID |
                                       RECONCILEF_NORESIDUESOK |
                                       RECONCILEF_OMITSELFRESIDUE |
                                       RECONCILEF_RESUMERECONCILIATION |
                                       RECONCILEF_YOUMAYDOTHEUPDATES |
                                       RECONCILEF_ONLYYOUWERECHANGED)
}
RECONCILEF;

#undef  INTERFACE
#define INTERFACE IReconcilableObject

DECLARE_INTERFACE_(IReconcilableObject, IUnknown)
{
   /* IUnknown methods */

   STDMETHOD(QueryInterface)(THIS_
                             REFIID riid,
                             PVOID *ppvObject) PURE;

   STDMETHOD_(ULONG, AddRef)(THIS) PURE;

   STDMETHOD_(ULONG, Release)(THIS) PURE;

   /* IReconcilableObject methods */

   STDMETHOD(Reconcile)(THIS_
                        IReconcileInitiator *pInitiator,
                        DWORD dwFlags,
                        HWND hwndOwner,
                        HWND hwndProgressFeedback,
                        ULONG ulcInput,
                        IMoniker **rgpmkOtherInput,
                        PLONG plOutIndex,
                        IStorage *pstgNewResidues,
                        PVOID pvReserved) PURE;

   STDMETHOD(GetProgressFeedbackMaxEstimate)(THIS_
                                             PULONG pulProgressMax) PURE;
};

#undef  INTERFACE
#define INTERFACE IBriefcaseInitiator

DECLARE_INTERFACE_(IBriefcaseInitiator, IUnknown)
{
   /* IUnknown methods */

   STDMETHOD(QueryInterface)(THIS_
                             REFIID riid,
                             PVOID *ppvObject) PURE;

   STDMETHOD_(ULONG, AddRef)(THIS) PURE;

   STDMETHOD_(ULONG, Release)(THIS) PURE;

   /* IBriefcaseInitiator methods */

   STDMETHOD(IsMonikerInBriefcase)(THIS_
                                   IMoniker *pmk) PURE;
};


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */


#endif   /* ! __RECONCIL_H__ */

