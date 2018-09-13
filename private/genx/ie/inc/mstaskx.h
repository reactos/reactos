//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1998.
//
//  File:       mstaskx.h
//
//  Contents:   this file solves the purpose to compile notify.idl
//              including the normal mstask.h results in compile error
//              because of multiple definitions of some data structurs
//
//  Classes:
//
//  Functions:
//
//  History:    1-09-1997   JohannP (Johann Posch)   Created
//
//  Note:       THIS FILE SHOULD EVENTUALLY GO AWAY!
//
//----------------------------------------------------------------------------

cpp_quote("#ifndef __mstask_h__")



cpp_quote("#define TASK_SUNDAY       (0x1)")
cpp_quote("#define TASK_MONDAY       (0x2)")
cpp_quote("#define TASK_TUESDAY      (0x4)")
cpp_quote("#define TASK_WEDNESDAY    (0x8)")
cpp_quote("#define TASK_THURSDAY     (0x10)")
cpp_quote("#define TASK_FRIDAY       (0x20)")
cpp_quote("#define TASK_SATURDAY     (0x40)")
cpp_quote("#define TASK_FIRST_WEEK   (1)")
cpp_quote("#define TASK_SECOND_WEEK  (2)")
cpp_quote("#define TASK_THIRD_WEEK   (3)")
cpp_quote("#define TASK_FOURTH_WEEK  (4)")
cpp_quote("#define TASK_LAST_WEEK    (5)")
cpp_quote("#define TASK_JANUARY      (0x1)")
cpp_quote("#define TASK_FEBRUARY     (0x2)")
cpp_quote("#define TASK_MARCH        (0x4)")
cpp_quote("#define TASK_APRIL        (0x8)")
cpp_quote("#define TASK_MAY          (0x10)")
cpp_quote("#define TASK_JUNE         (0x20)")
cpp_quote("#define TASK_JULY         (0x40)")
cpp_quote("#define TASK_AUGUST       (0x80)")
cpp_quote("#define TASK_SEPTEMBER    (0x100)")
cpp_quote("#define TASK_OCTOBER      (0x200)")
cpp_quote("#define TASK_NOVEMBER     (0x400)")
cpp_quote("#define TASK_DECEMBER     (0x800)")
cpp_quote("#define TASK_FLAG_INTERACTIVE                 (0x1)")
cpp_quote("#define TASK_FLAG_DELETE_WHEN_DONE            (0x2)")
cpp_quote("#define TASK_FLAG_DISABLED                    (0x4)")
cpp_quote("#define TASK_FLAG_HALT_ON_ERROR               (0x8)")
cpp_quote("#define TASK_FLAG_START_ONLY_IF_IDLE          (0x10)")
cpp_quote("#define TASK_FLAG_KILL_ON_IDLE_END            (0x20)")
cpp_quote("#define TASK_FLAG_DONT_START_IF_ON_BATTERIES  (0x40)")
cpp_quote("#define TASK_FLAG_KILL_IF_GOING_ON_BATTERIES  (0x80)")
cpp_quote("#define TASK_FLAG_RUN_ONLY_IF_DOCKED          (0x100)")
cpp_quote("#define TASK_FLAG_HIDDEN                      (0x200)")
cpp_quote("#define TASK_TRIGGER_FLAG_HAS_END_DATE            (0x1)")
cpp_quote("#define TASK_TRIGGER_FLAG_KILL_AT_DURATION_END    (0x2)")
cpp_quote("#define TASK_TRIGGER_FLAG_DISABLED                (0x4)")
cpp_quote("#define TASK_TRIGGER_FLAG_LAST_DAY_OF_MONTH       (0x8)")
cpp_quote("#define TASK_TRIGGER_FLAG_RUN_IF_CONNECTED_TO_INTERNET (0x10)")

cpp_quote("#define TASK_MAX_RUN_TIMES      ( 1440 )")

        typedef
        enum _TASK_TRIGGER_TYPE
            {   TASK_TIME_TRIGGER_ONCE  = 0,
                TASK_TIME_TRIGGER_DAILY = 1,
                TASK_TIME_TRIGGER_WEEKLY        = 2,
                TASK_TIME_TRIGGER_MONTHLYDATE   = 3,
                TASK_TIME_TRIGGER_MONTHLYDOW    = 4,
                TASK_EVENT_TRIGGER_ON_IDLE      = 5,
                TASK_EVENT_TRIGGER_AT_SYSTEMSTART       = 6,
                TASK_EVENT_TRIGGER_AT_LOGON     = 7
            }   TASK_TRIGGER_TYPE;

        typedef enum _TASK_TRIGGER_TYPE *PTASK_TRIGGER_TYPE;

        typedef struct  _DAILY
            {
            WORD DaysInterval;
            }   DAILY;

        typedef struct  _WEEKLY
            {
            WORD WeeksInterval;
            WORD rgfDaysOfTheWeek;
            }   WEEKLY;

        typedef struct  _MONTHLYDATE
            {
            DWORD rgfDays;
            WORD rgfMonths;
            }   MONTHLYDATE;

        typedef struct  _MONTHLYDOW
            {
            WORD wWhichWeek;
            WORD rgfDaysOfTheWeek;
            WORD rgfMonths;
            }   MONTHLYDOW;

        typedef union _TRIGGER_TYPE_UNION
            {
            DAILY Daily;
            WEEKLY Weekly;
            MONTHLYDATE MonthlyDate;
            MONTHLYDOW MonthlyDOW;
            }   TRIGGER_TYPE_UNION;

        typedef struct  _TASK_TRIGGER
            {
            WORD cbTriggerSize;
            WORD Reserved;
            WORD wBeginYear;
            WORD wBeginMonth;
            WORD wBeginDay;
            WORD wEndYear;
            WORD wEndMonth;
            WORD wEndDay;
            WORD wStartHour;
            WORD wStartMinute;
            DWORD MinutesDuration;
            DWORD MinutesInterval;
            DWORD rgFlags;
            TASK_TRIGGER_TYPE TriggerType;
            TRIGGER_TYPE_UNION Type;
            WORD wReservd2;
            WORD wRandomMinutesInterval;
            }   TASK_TRIGGER;

        typedef struct _TASK_TRIGGER *PTASK_TRIGGER;

cpp_quote("#endif // __mstask_h__")

