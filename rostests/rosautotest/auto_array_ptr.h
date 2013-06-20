/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Template similar to std::auto_ptr for arrays
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

template<typename Type>
class auto_array_ptr
{
private:
    Type* m_Ptr;

public:
    typedef Type element_type;

    /* Construct an auto_array_ptr from a pointer */
    explicit auto_array_ptr(Type* Ptr = 0) throw()
        : m_Ptr(Ptr)
    {
    }

    /* Construct an auto_array_ptr from an existing auto_array_ptr */
    auto_array_ptr(auto_array_ptr<Type>& Right) throw()
        : m_Ptr(Right.release())
    {
    }

    /* Destruct the auto_array_ptr and remove the corresponding array from memory */
    ~auto_array_ptr() throw()
    {
        delete[] m_Ptr;
    }

    /* Get the pointer address */
    Type* get() const throw()
    {
        return m_Ptr;
    }

    /* Release the pointer */
    Type* release() throw()
    {
        Type* Tmp = m_Ptr;
        m_Ptr = 0;

        return Tmp;
    }

    /* Reset to a new pointer */
    void reset(Type* Ptr = 0) throw()
    {
        if(Ptr != m_Ptr)
            delete[] m_Ptr;

        m_Ptr = Ptr;
    }

    /* Simulate all the functionality of real arrays by casting the auto_array_ptr to Type* on demand */
    operator Type*() const throw()
    {
        return m_Ptr;
    }
};
