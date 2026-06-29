//
// win_policies.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Wrapper functions that determine what policies should apply for the process in question.
//

#include <corecrt_internal.h>

template<typename TPolicy>
static typename TPolicy::policy_type __cdecl get_win_policy(typename TPolicy::appmodel_policy_type defaultValue)
{
    typename TPolicy::appmodel_policy_type appmodelPolicy = defaultValue;

    // Secure processes cannot load the appmodel DLL, so only attempt loading
    // policy information if this is not a secure process.
    if (!__acrt_is_secure_process())
    {
        TPolicy::appmodel_get_policy(&appmodelPolicy);
    }

    return TPolicy::appmodel_policy_to_policy_type(appmodelPolicy);
}

template<typename TPolicy>
static typename TPolicy::policy_type __cdecl get_cached_win_policy(typename TPolicy::appmodel_policy_type defaultValue)
{
    static long state_cache = 0;
    if (long const cached_state = __crt_interlocked_read(&state_cache))
    {
        return static_cast<typename TPolicy::policy_type>(cached_state);
    }

    typename TPolicy::appmodel_policy_type appmodelPolicy = defaultValue;

    // Secure processes cannot load the appmodel DLL, so only attempt loading
    // policy information if this is not a secure process.
    if (!__acrt_is_secure_process())
    {
        TPolicy::appmodel_get_policy(&appmodelPolicy);
    }

    typename TPolicy::policy_type const policyValue = TPolicy::appmodel_policy_to_policy_type(appmodelPolicy);

    long const cached_state = _InterlockedExchange(&state_cache, static_cast<long>(policyValue));
    if (cached_state)
    {
        _ASSERTE(cached_state == static_cast<long>(policyValue));
    }

    return policyValue;
}

// Determines whether ExitProcess() or TerminateProcess() should be used to end the process
extern "C" process_end_policy __cdecl __acrt_get_process_end_policy()
{
    struct process_end_policy_properties
    {
        typedef process_end_policy policy_type;
        typedef AppPolicyProcessTerminationMethod appmodel_policy_type;

        static policy_type appmodel_policy_to_policy_type(appmodel_policy_type const appmodelPolicy) throw()
        {
            if (appmodelPolicy == AppPolicyProcessTerminationMethod_TerminateProcess)
            {
                return process_end_policy_terminate_process;
            }
            else
            {
                return process_end_policy_exit_process;
            }
        }

        static LONG appmodel_get_policy(appmodel_policy_type* appmodelPolicy)
        {
            return __acrt_AppPolicyGetProcessTerminationMethodInternal(appmodelPolicy);
        }
    };

    return get_win_policy<process_end_policy_properties>(AppPolicyProcessTerminationMethod_ExitProcess);
}

// Determines whether RoInitialize() should be called when creating a thread
extern "C" begin_thread_init_policy __cdecl __acrt_get_begin_thread_init_policy()
{
    struct begin_thread_init_policy_properties
    {
        typedef begin_thread_init_policy policy_type;
        typedef AppPolicyThreadInitializationType appmodel_policy_type;

        static_assert(begin_thread_init_policy_unknown == 0, "Default value for cache must be 0");

        static policy_type appmodel_policy_to_policy_type(long const appmodelPolicy) throw()
        {
            if (appmodelPolicy == AppPolicyThreadInitializationType_InitializeWinRT)
            {
                return begin_thread_init_policy_ro_initialize;
            }
            else
            {
                return begin_thread_init_policy_none;
            }
        }

        static LONG appmodel_get_policy(appmodel_policy_type* appmodelPolicy)
        {
            return __acrt_AppPolicyGetThreadInitializationTypeInternal(appmodelPolicy);
        }
    };

    return get_cached_win_policy<begin_thread_init_policy_properties>(AppPolicyThreadInitializationType_None);
}

// Determines whether we should attempt to display assert dialog
extern "C" developer_information_policy __cdecl __acrt_get_developer_information_policy()
{
    struct developer_information_policy_properties
    {
        typedef developer_information_policy policy_type;
        typedef AppPolicyShowDeveloperDiagnostic appmodel_policy_type;

        static_assert(developer_information_policy_unknown == 0, "Default value for cache must be 0");

        static policy_type appmodel_policy_to_policy_type(long const appmodelPolicy) throw()
        {
            if (appmodelPolicy == AppPolicyShowDeveloperDiagnostic_ShowUI)
            {
                return developer_information_policy_ui;
            }
            else
            {
                return developer_information_policy_none;
            }
        }

        static LONG appmodel_get_policy(appmodel_policy_type* appmodelPolicy)
        {
            return __acrt_AppPolicyGetShowDeveloperDiagnosticInternal(appmodelPolicy);
        }
    };

    return get_cached_win_policy<developer_information_policy_properties>(AppPolicyShowDeveloperDiagnostic_ShowUI);
}

// Determines what type of windowing model technology is available
extern "C" windowing_model_policy __cdecl __acrt_get_windowing_model_policy()
{
    struct windowing_model_policy_properties
    {
        typedef windowing_model_policy policy_type;
        typedef AppPolicyWindowingModel appmodel_policy_type;

        static_assert(windowing_model_policy_unknown == 0, "Default value for cache must be 0");

        static policy_type appmodel_policy_to_policy_type(long const appmodelPolicy) throw()
        {
            switch (appmodelPolicy)
            {
                case AppPolicyWindowingModel_ClassicDesktop:
                    return windowing_model_policy_hwnd;

                case AppPolicyWindowingModel_Universal:
                    return windowing_model_policy_corewindow;

                case AppPolicyWindowingModel_ClassicPhone:
                    return windowing_model_policy_legacyphone;

                case AppPolicyWindowingModel_None:
                default:
                    return windowing_model_policy_none;
            }
        }

        static LONG appmodel_get_policy(appmodel_policy_type* appmodelPolicy)
        {
            return __acrt_AppPolicyGetWindowingModelInternal(appmodelPolicy);
        }
    };

    return get_cached_win_policy<windowing_model_policy_properties>(AppPolicyWindowingModel_ClassicDesktop);
}
