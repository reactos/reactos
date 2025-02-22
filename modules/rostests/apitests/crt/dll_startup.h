/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Helper declarations for DLL construction/destruction test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#pragma once

struct counter_values
{
    int m_uninit_at_startup;
    int m_uninit;
    int m_counter;
    int static_construct_counter_at_startup;
    int static_construct_counter;
    int dtor_counter_at_detach;
    int dtor_counter;
};

typedef
void
WINAPI
SET_COUNTER_VALUES_POINTER(
    _Out_ struct counter_values *pcv);
