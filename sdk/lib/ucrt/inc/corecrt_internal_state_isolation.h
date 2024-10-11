//
// corecrt_internal_state_isolation.h
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Header for CRT state management.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#ifdef __cplusplus
extern "C++"
{
    namespace __crt_state_management
    {
        constexpr size_t state_index_count = 2;
        struct scoped_global_state_reset
        {
            scoped_global_state_reset() throw() { }
            ~scoped_global_state_reset() throw() { }
        }; // FIXME: Implement this
        template <typename T>
        class dual_state_global
        {
            T _value[2];
        public:
            T& value(__crt_cached_ptd_host& ptd) throw();
            T const& value(__crt_cached_ptd_host& ptd) const throw();
            T& value(void) throw() { return _value[0]; }
            T value_explicit(size_t index) throw() { return _value[index]; }
            T* dangerous_get_state_array() throw() { return _value; }
            void initialize(T) throw() { }
            template<typename T2> void uninitialize(T2) throw() { }
            template<typename T2, size_t N> void initialize_from_array(T2 const (&values)[N]) throw() { }
        };

        inline int get_current_state_index(__crt_scoped_get_last_error_reset const &last_error_reset)
        {
            return 0;
        }

        inline int get_current_state_index(void)
        {
            return 0;
        }
    }
}
#endif // __cplusplus

#define __acrt_select_exit_lock() __acrt_exit_lock
