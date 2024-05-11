//
// corecrt_internal_ptd_propagation.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Consistently querying for per-thread data puts a significant overhead
// on runtime of the UCRT. Instead of re-querying the PTD every time it is
// needed, the goal going forward is to propagate the PTD, locale, global state index,
// and errno information between function calls via an argument.
// This header contains support for PTD propagation, disables common
// macros that invoke errno, and provides internal-only functions that
// propagate the per-thread data.

#pragma once
#include <corecrt_internal.h>
#include <stdlib.h>

_CRT_BEGIN_C_HEADER

// To grab the PTD, we also must query the global state index. Both of the FlsGetValue calls must be protected
// from modifying the Win32 error state, so must be guarded by GetLastError()/SetLastError().
// This function is provided so that if we already know the global state index and already
// have a Win32 error guard object, we can avoid doing it again for getting the PTD.
__acrt_ptd* __cdecl __acrt_getptd_noexit_explicit(__crt_scoped_get_last_error_reset const&, size_t global_state_index);

#ifdef __cplusplus
extern "C++"
{
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // PTD Data Host
    //
    // Upon public function entry, one of these objects should be instantiated in order
    // to host all the per-thread data for the rest of the function call. If any per-thread
    // data is required, it will be requested once for the full runtime of the function.
    // Additionally, changes to errno and doserrno will be recorded here instead, so that
    // the actual errno value will only be updated once, and will never be queried.
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    class __crt_cached_ptd_host
    {   // The goal of this class is to minimize the number of calls to FlsGetValue()/SetLastError().

        // Each call into the UCRT can use this class to lazily get and update:
        // * The __acrt_ptd:
        //    * errno / doserrno
        //    * locale / multibyte info
        // * The global state index
        // while calling GetFlsValue the minimum number of times.
        //
        // -- PTD and Global State Index --
        // Upon the first request of the PTD, both of these are updated at once. The global state index
        // is required for getting the correct PTD, and both need to be guarded with
        // a __crt_scoped_get_last_error_reset to prevent affecting the Win32 error.
        // If the global state index is requested prior to the PTD, only the global state index is updated.

        // -- Locale and Multibyte Info --
        // If given a valid _locale_t during construction, it is used for the locale data.
        // If the locale has not yet been set by a call to setlocale(), the initial locale
        // data for the C locale is used instead.
        // Otherwise, we wait until get_locale() is first called before querying the PTD
        // and locking the locale from any other changes until the destructor is called
        // (i.e. when the UCRT function call is completed).

        // -- errno and _doserrno --
        // This can be accessed directly via get_raw_ptd()->_terrno / _tdoserrno, but
        // checking errno results between internal function calls
        // (for example, printf can call wctomb and then check errno)
        // is a significant source of overhead.
        // Instead, both errno and doserrno changes are recorded locally in this class
        // and the values in the PTD are only updated once this class is destroyed and the
        // UCRT function call is complete.
        // This means that if you must check errno after calling another function, if errno
        // is not set then no PTD access was required. Using this instead of accessing errno
        // directly removed all PTD access in many printf scenarios.

        // Do not pass this directly, use __crt_cached_ptd_host&.
        // When the PTD is queried by a child function, the parent will also not need to re-query.
    public:
        enum class locale_status : unsigned char
        {
            uninitialized,
            updated_on_construction,
            updated_via_ptd
        };

        explicit __crt_cached_ptd_host(_locale_t const locale = nullptr) throw()
            : _ptd(nullptr), _current_global_state_index_valid(false), _locale_status(locale_status::uninitialized)
        {
            if (locale)
            {
                _locale_pointers = *locale;
                _locale_status   = locale_status::updated_on_construction;
            }
            else if (!__acrt_locale_changed())
            {
                _locale_pointers = __acrt_initial_locale_pointers;
                _locale_status   = locale_status::updated_on_construction;
            }
        }

        ~__crt_cached_ptd_host() throw()
        {
            if (_locale_status == locale_status::updated_via_ptd)
            {
                // We only locked the PTD from locale propagation if we are using
                // the locale data from the PTD.
                __acrt_enable_global_locale_sync(_ptd); // The PTD must be valid if locale was updated via the PTD.
            }

            if (_current_errno.valid())
            {
                get_raw_ptd()->_terrno = _current_errno.unsafe_value();
            }

            if (_current_doserrno.valid())
            {
                get_raw_ptd()->_tdoserrno = _current_doserrno.unsafe_value();
            }
        }

        __crt_cached_ptd_host(__crt_cached_ptd_host const&) = delete;
        __crt_cached_ptd_host& operator=(__crt_cached_ptd_host const&) = delete;

        _locale_t get_locale() throw()
        {
            update_locale();
            return &_locale_pointers;
        }

        size_t get_current_global_state_index() throw()
        {
            return check_synchronize_global_state_index();
        }

        __acrt_ptd * get_raw_ptd() throw()
        {
            return check_synchronize_per_thread_data();
        }

        __acrt_ptd * get_raw_ptd_noexit() throw()
        {
            return try_synchronize_per_thread_data();
        }

        template <typename T>
        struct cached
        {
        public:
            cached() throw()
                : _valid(false)
            {}

            bool valid() const throw()
            {
                return _valid;
            }

            T set(T new_value) throw()
            {
                _valid = true;
                _value = new_value;
                return new_value;
            }

            T value_or(T const alternative) const throw()
            {
                if (_valid)
                {
                    return _value;
                }
                return alternative;
            }

            bool check(T const value) const throw()
            {
                return _valid && _value == value;
            }

            class guard
            {
            public:
                explicit guard(cached& parent) throw()
                : _parent(parent), _copy(parent), _enabled(true)
                {
                }

                ~guard() throw()
                {
                    if (_enabled)
                    {
                        _parent = _copy;
                    }
                }

                guard(guard const&) = delete;
                guard& operator=(guard const&) = delete;

                void disable() throw()
                {
                    _enabled = false;
                }

                void enable() throw()
                {
                    _enabled = true;
                }

            private:
                cached& _parent;
                cached  _copy;
                bool    _enabled;
            };

            guard create_guard() throw()
            {
                return guard(*this);
            }

            T unsafe_value() throw()
            {
                // Must check status beforehand.
                return _value;
            }

        private:
            cached(cached const&) = default;
            cached(cached&&)      = default;

            cached& operator=(cached const&) = default;
            cached& operator=(cached&&)      = default;

            T    _value;
            bool _valid;
        };

        auto& get_errno() throw()
        {
            return _current_errno;
        }

        auto& get_doserrno() throw()
        {
            return _current_doserrno;
        }

    private:
        __forceinline void update_locale() throw()
        {   // Avoid costs for function call if locale doesn't need to be updated.
            if (_locale_status == locale_status::uninitialized)
            {
                update_locale_slow();
            }
        }

        void update_locale_slow() throw()
        {
            __acrt_ptd * const ptd_ptr = get_raw_ptd();

            _locale_pointers.locinfo = ptd_ptr->_locale_info;
            _locale_pointers.mbcinfo = ptd_ptr->_multibyte_info;

            // _get_raw_ptd() will update _current_global_state_index
            __acrt_update_locale_info_explicit(
                ptd_ptr, &_locale_pointers.locinfo, _current_global_state_index
                );

            __acrt_update_multibyte_info_explicit(
                ptd_ptr, &_locale_pointers.mbcinfo, _current_global_state_index
                );

            if ((ptd_ptr->_own_locale & _PER_THREAD_LOCALE_BIT) == 0)
            {
                // Skip re-synchronization with the global locale to prevent the
                // locale from changing half-way through the call.
                __acrt_disable_global_locale_sync(ptd_ptr);
                _locale_status = locale_status::updated_via_ptd;
            }
        }

        __forceinline __acrt_ptd * check_synchronize_per_thread_data() throw()
        {
            if (_ptd == nullptr)
            {
                if (force_synchronize_per_thread_data() == nullptr)
                {
                    abort();
                }
            }
            return _ptd;
        }

        __forceinline __acrt_ptd * try_synchronize_per_thread_data() throw()
        {
            if (_ptd == nullptr)
            {
                return force_synchronize_per_thread_data();
            }

            return _ptd;
        }

        __acrt_ptd * force_synchronize_per_thread_data() throw()
        {   // This function should be called at most once per UCRT function call.
            // Update all per-thread variables to minimize number of GetLastError() calls.
            __crt_scoped_get_last_error_reset const last_error_reset;

            return _ptd = __acrt_getptd_noexit_explicit(
                last_error_reset,
                check_synchronize_global_state_index(last_error_reset)
                );
        }

        size_t check_synchronize_global_state_index() throw()
        {
            if (!_current_global_state_index_valid)
            {
                __crt_scoped_get_last_error_reset const last_error_reset;
                return force_synchronize_global_state_index(last_error_reset);
            }

            return _current_global_state_index;
        }

        size_t check_synchronize_global_state_index(__crt_scoped_get_last_error_reset const& last_error_reset) throw()
        {
            if (!_current_global_state_index_valid)
            {
                return force_synchronize_global_state_index(last_error_reset);
            }

            return _current_global_state_index;
        }

        size_t force_synchronize_global_state_index(__crt_scoped_get_last_error_reset const& last_error_reset) throw()
        {
            _current_global_state_index       = __crt_state_management::get_current_state_index(last_error_reset);
            _current_global_state_index_valid = true;

            return _current_global_state_index;
        }

        __acrt_ptd *                   _ptd;

        size_t                         _current_global_state_index;
        bool                           _current_global_state_index_valid;

        __crt_locale_pointers          _locale_pointers;
        locale_status                  _locale_status;

        cached<errno_t>       _current_errno;
        cached<unsigned long> _current_doserrno;
    };


    namespace __crt_state_management
    {
        // If we have already grabbed the PTD, then we also grabbed the current global state index
        // and can use the global state index cached inside the __crt_cached_ptd_host.

        template <typename T>
        T& dual_state_global<T>::value(__crt_cached_ptd_host& ptd) throw()
        {
            return _value[ptd.get_current_global_state_index()];
        }

        template <typename T>
        T const& dual_state_global<T>::value(__crt_cached_ptd_host& ptd) const throw()
        {
            return _value[ptd.get_current_global_state_index()];
        }
    }
} // extern "C++"
#endif // __cplusplus

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Validation Macros / Errno update functions
//
// All the UCRT _VALIDATE_* macros will set errno directly on error.
// These new validation macros use __crt_cached_ptd_host& instead to prevent overhead.
//
// If this header is included, then the old validation/errno macros are #undef-ed to
// to prevent accidental calling of errno.
//
// _ALLOW_OLD_VALIDATE_MACROS is provided as an escape hatch for files that need to include
// this header, but still have code paths that require the old validate macros.
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

#ifndef _ALLOW_OLD_VALIDATE_MACROS
    // Use this to allow the validate macros without PTD propagation in a source file.
    #undef _INVALID_PARAMETER

    #undef _VALIDATE_CLEAR_OSSERR_RETURN
    #undef _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE
    #undef _VALIDATE_RETURN
    #undef _VALIDATE_RETURN_ERRCODE
    #undef _VALIDATE_RETURN_ERRCODE_NOEXC
    #undef _VALIDATE_RETURN_NOERRNO
    #undef _VALIDATE_RETURN_NOEXC
    #undef _VALIDATE_RETURN_VOID

    #undef _ERRCHECK_SPRINTF
    #undef _VALIDATE_STREAM_ANSI_RETURN
    #undef _CHECK_FH_RETURN
    #undef _CHECK_FH_CLEAR_OSSERR_RETURN
    #undef _CHECK_FH_CLEAR_OSSERR_RETURN_ERRCODE
    #undef _VALIDATE_CLEAR_OSSERR_RETURN

    #undef errno
    #undef _doserrno

#endif // _ALLOW_OLD_VALIDATE_MACROS

// Validate macros whose counterparts are from internal_shared.h

#ifdef _DEBUG
    #define _UCRT_INVALID_PARAMETER(ptd, expr) _invalid_parameter_internal(expr, __FUNCTIONW__, __FILEW__, __LINE__, 0, ptd)
#else // _DEBUG
    #define _UCRT_INVALID_PARAMETER(ptd, expr) _invalid_parameter_internal(nullptr, nullptr, nullptr, 0, 0, ptd)
#endif // _DEBUG

#define _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, expr, errorcode, retexpr)      \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            (ptd).get_doserrno().set(0L);                                     \
            (ptd).get_errno().set((errorcode));                               \
            _UCRT_INVALID_PARAMETER((ptd), _CRT_WIDE(#expr));                  \
            return (retexpr);                                                  \
        }                                                                      \
    }

#define _UCRT_VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(ptd, expr, errorcode)       \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            (ptd).get_doserrno().set(0L);                                     \
            (ptd).get_errno().set((errorcode));                               \
            _UCRT_INVALID_PARAMETER((ptd), _CRT_WIDE(#expr));                  \
            return (errorcode);                                                \
        }                                                                      \
    }

#define _UCRT_VALIDATE_RETURN(ptd, expr, errorcode, retexpr)                   \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            (ptd).get_errno().set((errorcode));                               \
            _UCRT_INVALID_PARAMETER((ptd), _CRT_WIDE(#expr));                  \
            return (retexpr);                                                  \
        }                                                                      \
    }

#define _UCRT_VALIDATE_RETURN_ERRCODE(ptd, expr, errorcode)                    \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            (ptd).get_errno().set((errorcode));                               \
            _UCRT_INVALID_PARAMETER((ptd), _CRT_WIDE(#expr));                  \
            return (errorcode);                                                \
        }                                                                      \
    }

#define _UCRT_VALIDATE_RETURN_ERRCODE_NOEXC(ptd, expr, errorcode)              \
    {                                                                          \
        if (!(expr))                                                           \
        {                                                                      \
            return (ptd).get_errno().set((errorcode));                        \
        }                                                                      \
    }

#define _UCRT_VALIDATE_RETURN_NOERRNO(ptd, expr, retexpr)                      \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            _UCRT_INVALID_PARAMETER((ptd), _CRT_WIDE(#expr));                  \
            return (retexpr);                                                  \
        }                                                                      \
    }

#define _UCRT_VALIDATE_RETURN_NOEXC(ptd, expr, errorcode, retexpr)             \
    {                                                                          \
        if (!(expr))                                                           \
        {                                                                      \
            (ptd).get_errno().set((errorcode));                               \
            return (retexpr);                                                  \
        }                                                                      \
    }

#define _UCRT_VALIDATE_RETURN_VOID(ptd, expr, errorcode)                       \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            (ptd).get_errno().set((errorcode));                               \
            _UCRT_INVALID_PARAMETER((ptd), _CRT_WIDE(#expr));                  \
            return;                                                            \
        }                                                                      \
    }

// Validate macros whose counterparts are from corecrt_internal.h

#define _UCRT_VALIDATE_STREAM_ANSI_RETURN(ptd, stream, errorcode, retexpr)     \
    {                                                                          \
        __crt_stdio_stream const _Stream((stream));                            \
        int fn;                                                                \
        _UCRT_VALIDATE_RETURN((ptd), (                                         \
            (_Stream.is_string_backed()) ||                                    \
            (fn = _fileno(_Stream.public_stream()),                            \
                ((_textmode_safe(fn) == __crt_lowio_text_mode::ansi) &&        \
                !_tm_unicode_safe(fn)))),                                      \
            (errorcode), (retexpr))                                            \
    }

#define _UCRT_CHECK_FH_RETURN(ptd, handle, errorcode, retexpr)                 \
    {                                                                          \
        if ((handle) == _NO_CONSOLE_FILENO)                                    \
        {                                                                      \
            (ptd).get_errno().set((errorcode));                               \
            return (retexpr);                                                  \
        }                                                                      \
    }

#define _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN(ptd, handle, errorcode, retexpr)    \
    {                                                                          \
        if ((handle) == _NO_CONSOLE_FILENO)                                    \
        {                                                                      \
            (ptd).get_doserrno().set(0L);                                     \
            (ptd).get_errno().set((errorcode));                               \
            return (retexpr);                                                  \
        }                                                                      \
    }

#define _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN_ERRCODE(ptd, handle, retexpr)       \
    {                                                                          \
        if ((handle) == _NO_CONSOLE_FILENO)                                    \
        {                                                                      \
            (ptd).get_doserrno().set(0L);                                     \
            return (retexpr);                                                  \
        }                                                                      \
    }

#define _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, expr, errorcode, retexpr)      \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            (ptd).get_doserrno().set(0L);                                     \
            (ptd).get_errno().set((errorcode));                               \
            _UCRT_INVALID_PARAMETER((ptd), _CRT_WIDE(#expr));                  \
            return (retexpr);                                                  \
        }                                                                      \
    }

_CRT_END_C_HEADER
