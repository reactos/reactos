/*
 * @(#)Measure.hxx 1.0 3/24/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _CORE_UTIL_MEASURE
#define _CORE_UTIL_MEASURE

DEFINE_CLASS(Measure);
DEFINE_CLASS(String);

/**
 * This is a general purpose Measure * object to allow store
 * pixel or inch, absolute or relative (%) measurements.
 *
 * @author  Istvan Cseri
 * @version 1.0, 3/24/97
 */

// public: virtual class Measure
class Measure : public Base
{
    DECLARE_CLASS_MEMBERS(Measure, Base);
    DECLARE_CLASS_INSTANCE(Measure, Base);

    /**
     * Value;
     */
    private: int value;
    /**
     * Type of measure (for now only absolute or relative pixel...)
     */
    private: byte type;

    public: enum
            {
                _ABSOLUTE = 0,
                _RELATIVE = 1,
            };

    public: Measure(int value = 0, byte type = _ABSOLUTE)
    {
        this->value = value;
        this->type = type;
    }

    public: virtual int getValue()
    {
        return value;
    }

    public: virtual int getValue(int base)
    {
        if (type == _ABSOLUTE)
        {
            return value;
        }
        else
        {
            return (int)((__int64)base * value / 100);
        }
    }

    public: virtual byte getType()
    {
        return type;
    }

    public: virtual bool isPixel()
    {
        return type == _ABSOLUTE;
    }

    public: virtual bool isPercent()
    {
        return type == _RELATIVE;
    }

    public: static Measure * parseMeasure(String * s);

    public: virtual bool equals(Object * obj);

    public: virtual int hashCode()
    {
        return value ^ type;
    }

    public: virtual String * toString();
};

#endif _CORE_UTIL_MEASURE

