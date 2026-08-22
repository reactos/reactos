// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <functional>
#include <type_traits>
#include "DelayCall.h"
#include "shared\win32error.h"

namespace wpf {
    namespace util {
        namespace DynamicCall {

            /// <summary>
            /// Common LoadLibraryEx flags
            /// </summary>
            enum class LoadLibraryFlags : DWORD
            {
                None = 0x00000000,
                DontResolveDllReferences = 0x00000001,
                LoadIgnoreCodeAuthzLevel = 0x00000010,
                LoadLibraryAsDataFile = 0x00000002,
                LoadLibraryAsDataFileExclusive = 0x00000040,
                LoadLibraryAsImageResource = 0x00000020,
                LoadLibrarySearchApplicationDir = 0x00000200,
                LoadLibrarySearchDefaultDirs = 0x00001000,
                LoadLibrarySearchDllLoadDir = 0x00000100,
                LoadLibrarySearchSystem32 = 0x00000800,
                LoadLibrarySearchUserDirs = 0x00000400,
                LoadWithAlteredSearchPath = 0x00000008,
            };

            inline LoadLibraryFlags operator|(LoadLibraryFlags lhs, LoadLibraryFlags rhs)
            {
                return static_cast<LoadLibraryFlags>(static_cast<std::underlying_type<LoadLibraryFlags>::type>(lhs) | static_cast<std::underlying_type<LoadLibraryFlags>::type>(rhs));
            }

            inline LoadLibraryFlags& operator|=(LoadLibraryFlags& lhs, LoadLibraryFlags rhs)
            {
                lhs = lhs | rhs;
                return lhs;
            }

            inline LoadLibraryFlags operator~(LoadLibraryFlags val)
            {
                return static_cast<LoadLibraryFlags>(~static_cast<std::underlying_type<LoadLibraryFlags>::type>(val));
            }

            inline LoadLibraryFlags operator&(LoadLibraryFlags lhs, LoadLibraryFlags rhs)
            {
                return static_cast<LoadLibraryFlags>(static_cast<std::underlying_type<LoadLibraryFlags>::type>(lhs) & static_cast<std::underlying_type<LoadLibraryFlags>::type>(rhs));
            }

            inline LoadLibraryFlags& operator&=(LoadLibraryFlags& lhs, LoadLibraryFlags rhs)
            {
                lhs = lhs & rhs;
                return lhs;
            }

            /// <summary>
            ///  An HMODULE abstraction with automatic support for
            /// LoadLibraryEx, detection of KB2533623 which added
            /// support for certain new flags, and ability to load
            /// function pointers dynamically.
            /// </summary>
            class ModuleHandle
            {
            private:
                HMODULE m_hModule = nullptr;

            private:
                /// <remarks>
                /// KB2533633 is required to use LOAD_LIBRARY_SEARCH_APPLICATION_DIR, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS, 
                /// LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR, LOAD_LIBRARY_SEARCH_SYSTEM32, LOAD_LIBRARY_SEARCH_USER_DIRS flags 
                /// with LoadLibraryEx
                /// </remarks>
                inline static bool IsKb2533623OrGreater()
                {
                    bool success = false;
                    try
                    {
                        auto pModule = std::make_shared<ModuleHandle>("kernel32.dll");
                        auto pAddDllDirectory = pModule->GetFunction<DLL_DIRECTORY_COOKIE(WINAPI*)(PCWSTR)>("AddDllDirectory");

                        success = (pAddDllDirectory != nullptr);
                    }
                    catch (...)
                    {

                    }

                    return success;
                }

                /// <summary>
                /// Removes KB2533623 dependent LoadLibraryEx flags that would cause
                /// failure when used on unsupported platforms
                /// </summary>
                inline static LoadLibraryFlags SanitizeFlags(LoadLibraryFlags flags)
                {
                    const LoadLibraryFlags KB2533623DependentFlags =
                        LoadLibraryFlags::LoadLibrarySearchApplicationDir |
                        LoadLibraryFlags::LoadLibrarySearchDefaultDirs |
                        LoadLibraryFlags::LoadLibrarySearchDllLoadDir |
                        LoadLibraryFlags::LoadLibrarySearchSystem32 |
                        LoadLibraryFlags::LoadLibrarySearchUserDirs;

                    return IsKb2533623OrGreater() ? flags : flags & ~KB2533623DependentFlags;
                }

                inline FARPROC GetFarProc(std::string method)
                {
                    auto proc = ::GetProcAddress(m_hModule, method.c_str());
                    if (proc == nullptr)
                    {
                        throw wpf::util::exceptions::win32error();
                    }

                    return proc;
                }

                inline static HMODULE LoadModule(std::string module)
                {
                    return LoadLibraryA(module.c_str());
                }

                inline static HMODULE LoadModule(std::wstring module)
                {
                    return LoadLibraryW(module.c_str());
                }

                inline static HMODULE LoadModuleEx(std::string module, LoadLibraryFlags flags)
                {
                    flags = SanitizeFlags(flags);
                    return LoadLibraryExA(module.c_str(), nullptr, static_cast<DWORD>(flags));
                }

                inline static HMODULE LoadModuleEx(std::wstring module, LoadLibraryFlags flags)
                {
                    flags = SanitizeFlags(flags);
                    return LoadLibraryExW(module.c_str(), nullptr, static_cast<DWORD>(flags));
                }

                inline void ThrowOnFailure()
                {
                    if (m_hModule == nullptr)
                    {
                        throw wpf::util::exceptions::win32error();
                    }
                }

            public:
                inline ModuleHandle(std::string module)
                    : m_hModule(LoadModule(module))
                {
                    ThrowOnFailure();
                }

                inline ModuleHandle(std::wstring module)
                    : m_hModule(LoadModule(module))
                {
                    ThrowOnFailure();
                }

                inline ModuleHandle(std::string module, LoadLibraryFlags flags)
                    : m_hModule(LoadModuleEx(module, flags))
                {
                    ThrowOnFailure();
                }

                inline ModuleHandle(std::wstring module, LoadLibraryFlags flags)
                    : m_hModule(LoadModuleEx(module, flags))
                {
                    ThrowOnFailure();
                }

                inline ~ModuleHandle()
                {
                    if (m_hModule != nullptr)
                    {
                        FreeLibrary(m_hModule);
                        m_hModule = nullptr;
                    }
                }

                template<typename Method>
                inline Method GetFunction(std::string method)
                {
                    auto proc = GetFarProc(method);
                    return reinterpret_cast<Method>(proc);
                }
            };
        }
    }
}
