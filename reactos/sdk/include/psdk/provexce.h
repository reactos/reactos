#pragma once

#ifndef _PROVIDER_EXCEPT_H
#define _PROVIDER_EXCEPT_H

class CHeap_Exception
{
public:
    enum HEAP_ERROR
    {
        E_ALLOCATION_ERROR = 0 ,
        E_FREE_ERROR
    };

    CHeap_Exception(HEAP_ERROR e) : m_Error(e) {}
    ~CHeap_Exception() {}

    HEAP_ERROR GetError() { return m_Error ; }
private:
    HEAP_ERROR m_Error;
};

#endif
