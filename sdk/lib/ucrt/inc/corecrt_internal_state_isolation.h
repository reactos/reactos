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
#ifdef _CRT_GLOBAL_STATE_ISOLATION
#error FIXME: Global state isolation is not implemented yet
        constexpr size_t state_index_count = 2;
#else
        constexpr size_t state_index_count = 1;
#endif

        struct scoped_global_state_reset
        {
            scoped_global_state_reset() throw() { }
            ~scoped_global_state_reset() throw() { }
        };

        template <typename T>
        class dual_state_global
        {
            T _value[state_index_count];

        public:

            // Implemented in corecrt_internal_ptd_propagation.h
            T& value(__crt_cached_ptd_host& ptd) throw();
            T const& value(__crt_cached_ptd_host& ptd) const throw();

            T& value(void) throw()
            {
                return _value[0];
            }

            T value_explicit(size_t index) throw()
            {
                return _value[index];
            }

            T* dangerous_get_state_array() throw()
            {
                return _value;
            }

            void initialize(T x) throw()
            {
                _value[0] = x;
            }

            template<typename T2>
            void uninitialize(T2 Lambda) throw()
            {
                Lambda(_value[0]);
            }

            template<typename T2>
            void initialize_from_array(T2 (&values)[state_index_count]) throw()
            {
                for (size_t i = 0; i < state_index_count; i++)
                {
                    _value[i] = values[i];
                }
            }
        };

        inline size_t get_current_state_index(__crt_scoped_get_last_error_reset const &last_error_reset)
        {
            return 0;
        }

        inline size_t get_current_state_index(void)
        {
            return 0;
        }
    }
}
#endif // __cplusplus

#define __acrt_select_exit_lock() __acrt_exit_lock
