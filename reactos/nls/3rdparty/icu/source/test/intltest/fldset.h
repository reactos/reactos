/*
************************************************************************
* Copyright (c) 2007, International Business Machines
* Corporation and others.  All Rights Reserved.
************************************************************************
*/
#ifndef FLDSET_H_
#define FLDSET_H_

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/calendar.h"
#include "unicode/ucal.h"
#include "unicode/udat.h"
#include "unicode/udbgutil.h"
#include "unicode/dbgutil.h"
#include "unicode/unistr.h"

#define U_FIELDS_SET_MAX  64

class FieldsSet {
    protected:
        /**
         * subclass interface
         * @param whichEnum which enumaration value goes with this set. Will be used to calculate str values and also enum size.
         */
        FieldsSet(UDebugEnumType whichEnum);
        
        /**
         * subclass interface - no enum tie-in
         * @param fieldCount how many fields this can hold.
         */
        FieldsSet(int32_t fieldsCount);

    public:
      /**
       * @param other "expected" set to match against
       * @param status - will return invalid argument if sets are not the same size
       * @return a formatted string listing which fields are set in 
       *   this, with the comparison made agaainst those fields in other.
       */
      UnicodeString diffFrom(const FieldsSet& other, UErrorCode &status) const;
      
    public:
      /**
       * @param str string to parse
       * @param status formatted string for status
       */
      int32_t parseFrom(const UnicodeString& str, UErrorCode& status) { return parseFrom(str,NULL,status); }
      
    public:
      int32_t parseFrom(const UnicodeString& str, const FieldsSet& inheritFrom, UErrorCode& status) { return parseFrom(str, &inheritFrom, status); }
      
      int32_t parseFrom(const UnicodeString& str, const 
              FieldsSet* inheritFrom, UErrorCode& status);
      
    protected:
      /**
       * Callback interface for subclass.
       * This function is called when parsing a field name, such as "MONTH"  in "MONTH=4".
       * Base implementation is to lookup the enum value using udbg_* utilities, or else as an integer if
       * enum is not available.
       * 
       * If there is a special directive, the implementer can catch it here and return -1 after special processing completes.
       * 
       * @param inheritFrom the set inheriting from - may be null.
       * @param name the field name (key side)
       * @param substr the string in question (value side)
       * @param status error status - set to error for failure.
       * @return field number, or negative if field should be skipped.
       */
      virtual int32_t handleParseName(const FieldsSet* inheritFrom, const UnicodeString& name, const UnicodeString& substr, UErrorCode& status);

      /**
       * Callback interface for subclass.
       * Base implementation is to call parseValueDefault(...)
       * @param inheritFrom the set inheriting from - may be null.
       * @param field which field is being parsed
       * @param substr the string in question (value side)
       * @param status error status - set to error for failure.
       * @see parseValueDefault
       */
      virtual void handleParseValue(const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status);
                
      /**
       * the default implementation for handleParseValue.
       * Base implementation is to parse a decimal integer value, or inherit from inheritFrom if the string is 0-length.
       * Implementations of this function should call set(field,...) on successful parse.
       * @see handleParseValue
       */
      void parseValueDefault(const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status);      


      /**
       * convenience implementation for handleParseValue
       * attempt to load a value from an enum value using udbg_enumByString()
       * if fails, will call parseValueDefault()
       * @see handleParseValue
       */
      void parseValueEnum(UDebugEnumType type, const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status);

    private:
      FieldsSet();
      
      void construct(UDebugEnumType whichEnum, int32_t fieldCount);

    public:
     virtual ~FieldsSet();
    
    void clear();
    void clear(int32_t field);
    void set(int32_t field, int32_t amount);
    UBool isSet(int32_t field) const;
    int32_t get(int32_t field) const;
    
    UBool isSameType(const FieldsSet& other) const;
    int32_t fieldCount() const;
    
    
    protected:
       int32_t fValue[U_FIELDS_SET_MAX];
       UBool fIsSet[U_FIELDS_SET_MAX];
    protected:
       int32_t fFieldCount;
       UDebugEnumType fEnum;
};

/** ------- Calendar Fields Set -------- **/
class CalendarFieldsSet : public FieldsSet {
    public:
        CalendarFieldsSet();
        virtual ~CalendarFieldsSet();

//        void clear(UCalendarDateFields field) { clear((int32_t)field); }
//        void set(UCalendarDateFields field, int32_t amount) { set ((int32_t)field, amount); }

//        UBool isSet(UCalendarDateFields field) const { return isSet((int32_t)field); }
//        int32_t get(UCalendarDateFields field) const { return get((int32_t)field); }

        /**
         * @param matches fillin to hold any fields different. Will have the calendar's value set on them.
         * @return true if the calendar matches in these fields.
         */
        UBool matches(Calendar *cal, CalendarFieldsSet &diffSet,
                UErrorCode& status) const;

        /**
         * set the specified fields on this calendar. Doesn't clear first. Returns any errors the cale 
         */
        void setOnCalendar(Calendar *cal, UErrorCode& status) const;

        
    protected:
        void handleParseValue(const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status);
};

/**
 * This class simply implements a set of date and time styles
 * such as DATE=SHORT  or TIME=SHORT,DATE=LONG
 */
class DateTimeStyleSet : public FieldsSet {
    public:
        DateTimeStyleSet();
        virtual ~DateTimeStyleSet();

        
        /** 
         * @return the date style, or UDAT_NONE if not set
         */
        UDateFormatStyle getDateStyle() const;
        
        /** 
         * @return the time style, or UDAT_NONE if not set
         */
        UDateFormatStyle getTimeStyle() const;

        
    protected:
        void handleParseValue(const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status);
        int32_t handleParseName(const FieldsSet* inheritFrom, const UnicodeString& name, const UnicodeString& substr, UErrorCode& status);
};


#endif /*!UCONFIG_NO_FORMAT*/
#endif /*FLDSET_H_*/
