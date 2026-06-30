// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains Dpi related utility functions
//
//-----------------------------------------------------------------------------

#pragma once

#include <Windows.h>
#include "DynamicCall\DelayCall.h"

/// <summary>
/// Generates a string literal representation
/// of a variable name.
/// </summary>
#define NAME_OF(X) #X

namespace wpf
{
    namespace util
    {
        /// <remarks>
        /// Declaring a types as a template allows the definition of
        /// statics <see cref="DpiUtilT::user32_dll"/> in the header (as against defining in another compilation unit)
        /// without violating ODR
        /// </remarks>
        template<typename = void>
        class DpiUtilT
        {
            using LoadLibraryFlags = wpf::util::DynamicCall::LoadLibraryFlags;
            using DynCall = wpf::util::DynamicCall::DynCall;

            static const char* user32_dll;

        public:
            /// <summary>
            /// Equivalent to <see cref="GetThreadDpiAwarenessContext"/>
            /// </summary>
            static DPI_AWARENESS_CONTEXT GetThreadDpiAwarenessContext()
            {
                static bool fEntryPointNotFound = false;

                DPI_AWARENESS_CONTEXT dpiContext = nullptr;
                if (!fEntryPointNotFound)
                {
                    try
                    {
                        dpiContext = DynCall::InvokeEx<DPI_AWARENESS_CONTEXT(WINAPI*)()>(
                            user32_dll,
                            NAME_OF(GetThreadDpiAwarenessContext),
                            LoadLibraryFlags::LoadLibrarySearchSystem32);
                    }
                    catch (const std::exception&)
                    {
                        fEntryPointNotFound = true;
                    }
                }

                return dpiContext;
            }

            /// <summary>
            /// Equivalent to <see cref="IsValidDpiAwarenessContext"/>
            /// </summary>
            static bool IsValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext)
            {
                static bool fEntryPointNotFound = false;

                bool isValid = false;
                if (!fEntryPointNotFound)
                {
                    if (dpiContext != nullptr)
                    {
                        try
                        {
                            isValid =
                                !!DynCall::InvokeEx<BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT)>(
                                    user32_dll,
                                    NAME_OF(IsValidDpiAwarenessContext),
                                    LoadLibraryFlags::LoadLibrarySearchSystem32,
                                    dpiContext);
                        }
                        catch (const std::exception&)
                        {
                            fEntryPointNotFound = true;
                        }
                    }
                }

                return isValid;
            }

            /// <summary>
            /// Equivalent to <see cref="AreDpiAwarenessContextsEqual"/>
            /// </summary>
            static bool AreDpiAwarenessContextsEqual(DPI_AWARENESS_CONTEXT dpiContextA, DPI_AWARENESS_CONTEXT dpiContextB)
            {
                static bool fEntryPointNotFound = false;

                bool areEqual = false;
                if (!fEntryPointNotFound)
                {
                    if (IsValidDpiAwarenessContext(dpiContextA) && IsValidDpiAwarenessContext(dpiContextB))
                    {
                        try
                        {
                            areEqual =
                                !!DynCall::InvokeEx<BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT)>(
                                    user32_dll,
                                    NAME_OF(AreDpiAwarenessContextsEqual),
                                    LoadLibraryFlags::LoadLibrarySearchSystem32,
                                    dpiContextA,
                                    dpiContextB);
                        }
                        catch (const std::exception&)
                        {
                            fEntryPointNotFound = true;
                        }
                    }
                }

                return areEqual;
            }

            /// <summary>
            /// Equivalent to <see cref="GetDpiForSystem"/>
            /// </summary>
            static UINT GetDpiForSystem()
            {
                static bool fEntryPointNotFound = false;

                UINT systemDpi = 0;
                if (!fEntryPointNotFound)
                {
                    try
                    {
                        systemDpi = DynCall::InvokeEx<UINT(WINAPI*)()>(
                            user32_dll,
                            NAME_OF(GetDpiForSystem),
                            LoadLibraryFlags::LoadLibrarySearchSystem32);
                    }
                    catch (const std::exception&)
                    {
                        fEntryPointNotFound = true;
                    }
                }

                return systemDpi;
            }

            /// <summary>
            /// Equivalent to <see cref="GetWindowDpiAwarenessContext"/>
            /// </summary>
            static DPI_AWARENESS_CONTEXT GetWindowDpiAwarenessContext(HWND hWnd)
            {
                static bool fEntryPointNotFound = false;

                DPI_AWARENESS_CONTEXT dpiContext = nullptr;
                if (!fEntryPointNotFound)
                {
                    try
                    {
                        dpiContext = DynCall::InvokeEx<DPI_AWARENESS_CONTEXT(WINAPI*)(HWND)>(
                            user32_dll,
                            NAME_OF(GetWindowDpiAwarenessContext),
                            LoadLibraryFlags::LoadLibrarySearchSystem32,
                            hWnd);
                    }
                    catch (const std::exception&)
                    {
                        fEntryPointNotFound = true;
                    }
                }

                return dpiContext;
            }

            /// <summary>
            /// Equivalent to <see cref="SetThreadDpiAwarenessContext"/>
            /// </summary>
            static DPI_AWARENESS_CONTEXT SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext)
            {
                static bool fEntryPointNotFound = false;

                DPI_AWARENESS_CONTEXT oldDpiContext = nullptr;
                if (!fEntryPointNotFound)
                {
                    try
                    {
                        oldDpiContext = DynCall::InvokeEx<DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT)>(
                            user32_dll,
                            NAME_OF(SetThreadDpiAwarenessContext),
                            LoadLibraryFlags::LoadLibrarySearchSystem32,
                            dpiContext);
                    }
                    catch (const std::exception&)
                    {
                        fEntryPointNotFound = true;
                    }
                }

                return oldDpiContext;
            }
        };

        const char* DpiUtilT<void>::user32_dll = "user32.dll";
        typedef DpiUtilT<void> DpiUtil;

        /// <summary>
        /// An enumeration that maps to the DPI_AWARENESS_CONTEXT pseudo handles
        /// </summary>
        /// <remarks>
        /// This is an internal enumeration. There is no analogue
        /// for this in the Windows headers
        ///
        /// This a very important enum and these values should not
        /// be changed lightly.
        ///
        /// HwndTarget keeps track of its own DPI_AWARENESS_CONTEXT using this
        /// enum, and passes along this value directly to the renderer.
        ///
        /// Eventually, this is interpreted within DpiProvider::SetDpiAwarenessContext
        /// as a DPI_AWARENESS_CONTEXT (pseudo) handle. For this internal protocol
        /// to work correctly, the values used here need to remain in sync with
        /// (a) the values using in DpiProvider::SetDpiAwarenessContext and
        /// (b) the values used to initialize the DPI_AWARENESS_CONTEXT
        /// (pseudo) handles in the Windows headers.
        /// </remarks>
        enum class DpiAwarenessContextValue : int
        {
            /// <summary>
            /// Invalid value
            /// </summary>
            Invalid = 0,

            /// <summary>
            /// DPI_AWARENESS_CONTEXT_UNAWARE
            /// </summary>
            Unaware = -1,

            /// <summary>
            /// DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
            /// </summary>
            SystemAware = -2,

            /// <summary>
            /// DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
            /// </summary>
            PerMonitorAware = -3,

            /// <summary>
            /// DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
            /// </summary>
            PerMonitorAwareVersion2 = -4
        };

        /// <summary>
        /// An abstraction over <see cref="DPI_AWARENESS_CONTEXT"/> that maps pseudo-handles
        /// to <see cref="DpiAwarenessContextValue"/> enumeration values
        /// </summary>
        class DpiAwarenessContext
        {
        private:
            DPI_AWARENESS_CONTEXT m_dpiAwarenessContext;
            DpiAwarenessContextValue m_dpiAwarenessContextValue;

        public:
            inline DpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext) : 
                m_dpiAwarenessContext(dpiContext),
                m_dpiAwarenessContextValue(FindCanonicalValue(dpiContext))
            {
            }

            inline DpiAwarenessContext(DpiAwarenessContextValue dpiContextValue) :
                m_dpiAwarenessContextValue(dpiContextValue),
                m_dpiAwarenessContext(dpiContextValue != DpiAwarenessContextValue::Invalid ? reinterpret_cast<DPI_AWARENESS_CONTEXT>(dpiContextValue) : nullptr)
            {
            }

            inline DpiAwarenessContext(int context) :
                DpiAwarenessContext(FindCanonicalValue(context))
            {
            }

            /// <summary>
            /// Default constructor acquires current threads
            /// DPI awareness context
            /// </summary>
            inline DpiAwarenessContext() : DpiAwarenessContext(DpiUtil::GetThreadDpiAwarenessContext())
            {
            }

            inline ~DpiAwarenessContext() {}

            inline static std::vector<DpiAwarenessContextValue> GetValidDpiAwarenessContextValues()
            {
                static std::shared_ptr<std::vector<DpiAwarenessContextValue>> pValidDpiAwarenessContextValues;

                const std::vector<DpiAwarenessContextValue> allDpiAwarenessContextValues =
                {
                    DpiAwarenessContextValue::Unaware,
                    DpiAwarenessContextValue::SystemAware,
                    DpiAwarenessContextValue::PerMonitorAware,
                    DpiAwarenessContextValue::PerMonitorAwareVersion2
                };

                if (!pValidDpiAwarenessContextValues)
                {
                    pValidDpiAwarenessContextValues = std::make_shared<std::vector<DpiAwarenessContextValue>>();

                    for (auto dpiAwarenessContextValue : allDpiAwarenessContextValues)
                    {
                        if (DpiUtil::IsValidDpiAwarenessContext(reinterpret_cast<DPI_AWARENESS_CONTEXT>(dpiAwarenessContextValue)))
                        {
                            pValidDpiAwarenessContextValues->push_back(dpiAwarenessContextValue);
                        }
                    }
                }

                return *pValidDpiAwarenessContextValues;
            }

            inline bool IsValid() const
            {
                return m_dpiAwarenessContextValue != DpiAwarenessContextValue::Invalid;
            }

            inline operator bool() const
            {
                return IsValid();
            }

            inline DpiAwarenessContextValue GetDpiAwarenessContextValue() const
            {
                return m_dpiAwarenessContextValue;
            }

            inline operator DpiAwarenessContextValue() const
            {
                return GetDpiAwarenessContextValue();
            }

            inline DPI_AWARENESS_CONTEXT GetDpiAwarenessContext() const
            {
                return
                    IsValid()
                    ? reinterpret_cast<DPI_AWARENESS_CONTEXT>(m_dpiAwarenessContextValue)
                    : nullptr;
            }

            inline operator DPI_AWARENESS_CONTEXT() const
            {
                return GetDpiAwarenessContext();
            }

            static inline DpiAwarenessContextValue GetThreadDpiAwarenessContextValue()
            {
                return static_cast<DpiAwarenessContextValue>(DpiAwarenessContext());
            }


        private:

            inline static DpiAwarenessContextValue FindCanonicalValue(DPI_AWARENESS_CONTEXT dpiAwarenessContext)
            {
                auto canonicalValue = DpiAwarenessContextValue::Invalid;

                if (DpiUtil::IsValidDpiAwarenessContext(dpiAwarenessContext))
                {
                    for (auto dpiAwarenessContextValue : GetValidDpiAwarenessContextValues())
                    {
                        DPI_AWARENESS_CONTEXT canonicalDpiAwarenessContext = reinterpret_cast<DPI_AWARENESS_CONTEXT>(dpiAwarenessContextValue);
                        if (DpiUtil::AreDpiAwarenessContextsEqual(canonicalDpiAwarenessContext, dpiAwarenessContext))
                        {
                            canonicalValue = dpiAwarenessContextValue;
                            break;
                        }
                    }
                }

                return canonicalValue;
            }

            inline static DpiAwarenessContextValue FindCanonicalValue(int context)
            {
                auto dpiAwarenessContextValue = DpiAwarenessContextValue::Invalid;

                for (auto dpiContextValue : GetValidDpiAwarenessContextValues())
                {
                    if (static_cast<int>(dpiContextValue) == context)
                    {
                        dpiAwarenessContextValue = dpiContextValue;
                        break;
                    }
                }

                return dpiAwarenessContextValue;
            }
        };

        class DpiAwarenessScopeBase
        {
        protected:
            DPI_AWARENESS_CONTEXT m_desiredDpiContext = nullptr;
            DPI_AWARENESS_CONTEXT m_oldDpiContext = nullptr;

        public:
            inline DpiAwarenessScopeBase(DPI_AWARENESS_CONTEXT dpiContext)
                : m_desiredDpiContext(dpiContext)
            {
                if (DpiUtil::IsValidDpiAwarenessContext(m_desiredDpiContext))
                {
                    m_oldDpiContext = DpiUtil::SetThreadDpiAwarenessContext(m_desiredDpiContext);
                }
            }

            inline virtual ~DpiAwarenessScopeBase()
            {
                if (DpiUtil::IsValidDpiAwarenessContext(m_oldDpiContext))
                {
                    DpiUtil::SetThreadDpiAwarenessContext(m_oldDpiContext);
                    m_desiredDpiContext = nullptr;
                    m_desiredDpiContext = nullptr;
                }
            }
        };

        template<typename TDpiAwarenessScopeSource>
        class DpiAwarenessScope : public DpiAwarenessScopeBase
        {
        public:
            inline DpiAwarenessScope(TDpiAwarenessScopeSource source, std::function<DPI_AWARENESS_CONTEXT(TDpiAwarenessScopeSource)> extractor)
                : DpiAwarenessScopeBase(extractor(source))
            {
            }
        };

        template<>
        class DpiAwarenessScope<HWND> : public DpiAwarenessScopeBase
        {
        public:
            inline DpiAwarenessScope(HWND hWnd)
                : DpiAwarenessScopeBase(DpiUtil::GetWindowDpiAwarenessContext(hWnd))
            {
            }
        };

        template<>
        class DpiAwarenessScope<DpiAwarenessContext> : public DpiAwarenessScopeBase
        {
        public:
            DpiAwarenessScope(DpiAwarenessContext dpiContext)
                : DpiAwarenessScopeBase(dpiContext)
            {
            }
        };
    }
}


