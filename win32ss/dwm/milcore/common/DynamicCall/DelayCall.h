// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include <string>
#include <mutex>
#include <type_traits>
#include <map>

#include "ModuleHandle.h"
#include "shared\SehException.h"
#include "shared\win32error.h"

using SehGuard = wpf::util::exceptions::SehGuard;
using SehException = wpf::util::exceptions::SehException;

namespace wpf {
    namespace util {
        namespace DynamicCall {

            /// <summary>
            /// Identifies the return type of a function based on its
            /// signature
            /// </summary>
            template<typename Method, typename...Args>
            struct result_of
            {
                typedef
                    decltype(std::declval<Method>()(std::declval<Args>()...))
                    type;
            };

            /// <summary>
            /// Identifies a function pointer type
            /// Equivalent to <see cref="std::true_type"/> when <typeparam name="T"/> is a function pointer.
            /// </summary>
            template<typename T, bool = std::is_pointer<T>::value>
            struct is_function_ptr : public std::integral_constant<bool, std::is_function<typename std::remove_pointer<T>::type>::value>
            {
            };

            template<typename T>
            struct is_function_ptr<T, false> : std::false_type
            {
            };


            /// <summary>
            /// Module and function pointer cache abstraction
            /// </summary>
            class ModuleCache
            {
            private:

                /// <summary>
                /// Key for the cache - consists of module name and LoadLibraryEx flags
                /// </summary>
                struct ModuleCacheKey
                {
                    std::string ModuleName;
                    LoadLibraryFlags Flags;

                public:
                    inline ModuleCacheKey(std::string moduleName, LoadLibraryFlags dwFlags) :
                        ModuleName(moduleName), Flags(dwFlags)
                    {
                    }

                    inline ModuleCacheKey(std::string moduleName) :
                        ModuleCacheKey(moduleName, LoadLibraryFlags::None)
                    {
                    }

                    inline bool operator<(const ModuleCacheKey& other) const
                    {
                        if (ModuleName < other.ModuleName)
                        {
                            return true;
                        }

                        if (static_cast<DWORD>(Flags) < static_cast<DWORD>(other.Flags))
                        {
                            return true;
                        }

                        return false;
                    }
                };

                /// <summary>
                /// Cached value - consists of module handle and a list of function pointers
                /// loaded from within that module
                /// </summary>
                struct ModuleCacheValue
                {
                    std::shared_ptr<ModuleHandle> ModuleHandle;
                    std::map<std::string, void*> FunctionPointers;
                };

                /// <summary>
                /// The actual cache maintained as a key-value pair
                /// </summary>
                std::map<ModuleCacheKey, ModuleCacheValue> m_cache;

                /// <summary>
                /// Mutex to ensure that the only one thread can update the cache at
                /// any given time.
                /// </summary>
                /// <remarks>
                /// This is not a recursive mutex - i.e., a given thread can not re-acquire the
                /// mutex before releasing it first.
                /// </remarks>
                std::mutex m_lock;

            public:

                /// <summary>
                /// Tests whether a given (module name + LoadLibraryEx flags) combination
                /// has an entry in the cache
                /// </summary>
                /// <param name="strModuleName"></param>
                /// <param name="dwFlags"></param>
                /// <returns></returns>
                inline bool Contains(std::string strModuleName, LoadLibraryFlags dwFlags)
                {
                    ModuleCacheKey key(strModuleName, dwFlags);
                    if (m_cache.find(key) != m_cache.end())
                    {
                        return true;
                    }

                    return false;
                }

                // Retrieves a cached function pointer
                // The returned type can either be a specific function pointer type, or void*

                /// <summary>
                /// Retrieves a cached function pointer
                /// </summary>
                /// <remarks>
                /// The returned type can either be a specific function pointer type, or void*
                /// </remarks>
                template<typename MethodType, typename = typename std::enable_if<is_function_ptr<MethodType>::value || std::is_same<MethodType, void*>::value>::type>
                inline MethodType GetCachedFunction(std::string strModuleName, LoadLibraryFlags dwFlags, std::string strFunctionName)
                {
                    if (!Contains(strModuleName, dwFlags))
                    {
                        return nullptr;
                    }

                    ModuleCacheKey key(strModuleName, dwFlags);
                    ModuleCacheValue& value = m_cache[key];

                    if (value.FunctionPointers.find(strFunctionName) == value.FunctionPointers.end())
                    {
                        return nullptr;
                    }

                    return reinterpret_cast<MethodType>(value.FunctionPointers.at(strFunctionName));
                }

                /// <summary>
                /// Tests whether a function has an entry in the cache
                /// </summary>
                /// <param name="strModuleName">Module/DLL name</param>
                /// <param name="dwFlags">LoadLibraryEx flags used when the module was loaded</param>
                /// <param name="strFunctionName">Name of the function</param>
                /// <returns>True if an entry exists in the cache, otherwise false</returns>
                inline bool Contains(std::string strModuleName, LoadLibraryFlags dwFlags, std::string strFunctionName)
                {
                    return GetCachedFunction<void*>(strModuleName, dwFlags, strFunctionName) == nullptr;
                }

                /// <summary>
                /// Retrieves a cached function
                /// </summary>
                /// <returns>Pointer to the function, or nullptr if an entry is not found in the cache</returns>
                template <typename MethodType, typename = typename std::enable_if<is_function_ptr<MethodType>::value>::type>
                inline MethodType CacheFunction(std::string strModuleName, LoadLibraryFlags dwFlags, std::string strFunctionName)
                {
                    std::lock_guard<std::mutex> lock(m_lock);

                    // If this function has been inserted into the cache since the last check, 
                    // find and return it here.
                    MethodType func = GetCachedFunction<MethodType>(strModuleName, dwFlags, strFunctionName);
                    if (func != nullptr)
                    {
                        return func;
                    }

                    ModuleCacheKey key(strModuleName, dwFlags);
                    ModuleCacheValue& value = m_cache[key];

                    if (value.ModuleHandle == nullptr)
                    {
                        value.ModuleHandle = std::make_shared<ModuleHandle>(strModuleName, dwFlags);
                    }

                    func = value.ModuleHandle->GetFunction<MethodType>(strFunctionName);
                    value.FunctionPointers[strFunctionName] = reinterpret_cast<void*>(func);

                    return func;
                }
            };

            /// <summary>
            /// Base class for DelayCall routines
            /// </summary>
            class DelayCallBase
            {
            protected:
                /// <summary>
                /// Cache of loaded modules and function pointers
                /// </summary>
                static ModuleCache m_cache;

            protected:
                virtual ~DelayCallBase() {}

                /// <summary>
                /// Calls the function represented by a pointer <paramref name="function"/>
                /// </summary>
                template<typename MethodType, typename... Args>
                inline static auto Call(MethodType function, Args&&...args) -> typename result_of<MethodType, Args...>::type
                {
                    // Translates SEH exceptions in this scope to C++ exceptions
                    SehGuard sehGuard;

                    if (function == nullptr)
                    {
                        throw wpf::util::exceptions::win32error();
                    }

                    return function(std::forward<Args>(args)...);
                }
            };

            /// <summary>
            /// Implements dynamic dispatch of function calls using LoadLibrary(Ex) + GetProcAddress,
            /// and supports optional caching of function pointers discovered in this manner.
            /// </summary>
            template <bool fCache>
            class DelayCall;

            /// <summary>
            /// Template specialization of <see cref="DelayCall"/> with caching enabled
            /// </summary>
            /// <remarks>
            /// If the module or the function pointer are absent from the cache, then they are added to the cache. 
            /// </remarks>
            template<>
            class DelayCall<true> : protected DelayCallBase
            {
            public:
                /// <summary>
                /// Invokes a function <paramref name="function"/> from module <paramref name="module"/>, and returns
                /// the result if applicable.
                /// </summary>
                /// <typeparam name="MethodType">Function signature of <paramref name="function"/> in pointer form</typeparam>
                /// <typeparam name="Args">Variadic template parameter inferred from <paramref name="args"/></typeparam>
                /// <param name="module">Name of the module from which the function is to be discovered and loaded</param>
                /// <param name="method">Name of the function</param>
                /// <param name="dwFlags">LoadLibraryEx flags used when loading the module <paramref name="module"/></param>
                /// <param name="args">List of arguments passed to the function</param>
                /// <returns>The result of the function call</returns>
                /// <exception cref="std::system_error">
                /// If the module could not be loaded, or if the function pointer could not be obtained from the module, then the underlying
                /// Win32 error code is translated into a <see cref="std::system_error"/> and thrown
                /// </exception>
                /// <exception cref="SehException">
                /// If an SEH exception occurs during the execution of the function, then it is converted into an
                /// <see cref="SehException"/> and thrown
                /// </exception>
                /// <exception cref="std::exception">
                /// If a C++ <see cref="std::exception"/> or a derived exception type is encountered during function call,
                /// it is forwarded directly to the caller.
                /// </exception>
                template <typename MethodType, typename... Args>
                inline static auto InvokeEx(std::string module, std::string method, LoadLibraryFlags dwFlags, Args&&...args) -> typename result_of<MethodType, Args...>::type
                {
                    MethodType function = m_cache.GetCachedFunction<MethodType>(module, dwFlags, method);
                    if (function == nullptr)
                    {
                        function = m_cache.CacheFunction<MethodType>(module, dwFlags, method);
                    }

                    return Call(function, std::forward<Args>(args)...);
                }

                /// <summary>
                /// Invokes a function <paramref name="function"/> from module <paramref name="module"/>, and returns
                /// the result if applicable.
                /// </summary>
                /// <typeparam name="MethodType">Function signature of <paramref name="function"/> in pointer form</typeparam>
                /// <typeparam name="Args">Variadic template parameter inferred from <paramref name="args"/></typeparam>
                /// <param name="module">Name of the module from which the function is to be discovered and loaded</param>
                /// <param name="method">Name of the function</param>
                /// <param name="args">List of arguments passed to the function</param>
                /// <returns>The result of the function call</returns>
                /// <exception cref="std::system_error">
                /// If the module could not be loaded, or if the function pointer could not be obtained from the module, then the underlying
                /// Win32 error code is translated into a <see cref="std::system_error"/> and thrown
                /// </exception>
                /// <exception cref="SehException">
                /// If an SEH exception occurs during the execution of the function, then it is converted into an
                /// <see cref="SehException"/> and thrown
                /// </exception>
                /// <exception cref="std::exception">
                /// If a C++ <see cref="std::exception"/> or a derived exception type is encountered during function call,
                /// it is forwarded directly to the caller.
                /// </exception>
                template <typename MethodType, typename... Args>
                inline static auto Invoke(std::string module, std::string method, Args&&... args) -> typename result_of<MethodType, Args...>::type
                {
                    return InvokeEx<MethodType, Args...>(module, method, LoadLibraryFlags::None, std::forward<Args>(args)...);
                }
            };

            /// <summary>
            /// Template specialization of <see cref="DelayCall"/> without caching enabled
            /// </summary>
            /// <remarks>
            /// If the function is found in the cache, then it is used. If it is not found in the cache, then the
            /// function pointer is loaded from the module (and the module itself is loaded if necessary), but the
            /// module and the function pointer are not cached anew.
            /// </remarks>
            template<>
            class DelayCall<false> : protected DelayCallBase
            {
            public:
                /// <summary>
                /// Invokes a function <paramref name="function"/> from module <paramref name="module"/>, and returns
                /// the result if applicable.
                /// </summary>
                /// <typeparam name="MethodType">Function signature of <paramref name="function"/> in pointer form</typeparam>
                /// <typeparam name="Args">Variadic template parameter inferred from <paramref name="args"/></typeparam>
                /// <param name="module">Name of the module from which the function is to be discovered and loaded</param>
                /// <param name="method">Name of the function</param>
                /// <param name="dwFlags">LoadLibraryEx flags used when loading the module <paramref name="module"/></param>
                /// <param name="args">List of arguments passed to the function</param>
                /// <returns>The result of the function call</returns>
                /// <exception cref="std::system_error">
                /// If the module could not be loaded, or if the function pointer could not be obtained from the module, then the underlying
                /// Win32 error code is translated into a <see cref="std::system_error"/> and thrown
                /// </exception>
                /// <exception cref="SehException">
                /// If an SEH exception occurs during the execution of the function, then it is converted into an
                /// <see cref="SehException"/> and thrown
                /// </exception>
                /// <exception cref="std::exception">
                /// If a C++ <see cref="std::exception"/> or a derived exception type is encountered during function call,
                /// it is forwarded directly to the caller.
                /// </exception>
                template <typename MethodType, typename... Args>
                inline static auto InvokeEx(std::string module, std::string method, LoadLibraryFlags dwFlags, Args&&...args) -> typename result_of<MethodType, Args...>::type
                {
                    std::shared_ptr<ModuleHandle> pModule;

                    // Query the cache first. If the function is not present in the cache, then 
                    // resort to non-cached LoadLibrary + GetProcAddress
                    MethodType function = m_cache.GetCachedFunction<MethodType>(module, dwFlags, method);
                    if (function == nullptr)
                    {
                        pModule = std::make_shared<ModuleHandle>(module);
                        function = pModule->GetFunction<MethodType>(method);
                    }

                    return Call(function, std::forward<Args>(args)...);
                }

                /// <summary>
                /// Invokes a function <paramref name="function"/> from module <paramref name="module"/>, and returns
                /// the result if applicable.
                /// </summary>
                /// <typeparam name="MethodType">Function signature of <paramref name="function"/> in pointer form</typeparam>
                /// <typeparam name="Args">Variadic template parameter inferred from <paramref name="args"/></typeparam>
                /// <param name="module">Name of the module from which the function is to be discovered and loaded</param>
                /// <param name="method">Name of the function</param>
                /// <param name="args">List of arguments passed to the function</param>
                /// <returns>The result of the function call</returns>
                /// <exception cref="std::system_error">
                /// If the module could not be loaded, or if the function pointer could not be obtained from the module, then the underlying
                /// Win32 error code is translated into a <see cref="std::system_error"/> and thrown
                /// </exception>
                /// <exception cref="SehException">
                /// If an SEH exception occurs during the execution of the function, then it is converted into an
                /// <see cref="SehException"/> and thrown
                /// </exception>
                /// <exception cref="std::exception">
                /// If a C++ <see cref="std::exception"/> or a derived exception type is encountered during function call,
                /// it is forwarded directly to the caller.
                /// </exception>
                template <typename MethodType, typename... Args>
                inline static auto Invoke(std::string module, std::string method, Args&&...args) -> typename result_of<MethodType, Args...>::type
                {
                    return InvokeEx<MethodType, Args...>(module, method, LoadLibraryFlags::None, std::forward<Args>(args)...);
                }
            };

            typedef DelayCall<true> DynCall;
            typedef DelayCall<false> DynCallNoCache;
        }
    }
}
