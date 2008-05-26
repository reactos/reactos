#ifndef TIMER_COMMAND_CALLBACK_H__ //timer_command_callback.h
#define TIMER_COMMAND_CALLBACK_H__

#include "conf_parser.h"
#include "timer_callback.h"

namespace System_
{

    class TimerCommandCallback : public TimerCallback
    {
    public:
//---------------------------------------------------------------
///
/// TimerCommandCallback
///
/// Description: constructor of class TimerCommandCallback
///
/// @param conf_parser contains configuration data

          TimerCommandCallback(Sysreg_::ConfigParser &conf_parser);

//---------------------------------------------------------------
///
/// ~TimerCommandCallback
///
/// Description: destructor of class TimerCommandCallback

          virtual ~TimerCommandCallback();

//---------------------------------------------------------------
///
/// initialize
///
/// Description: initializes the callback object

          virtual bool initialize();

//---------------------------------------------------------------
///
/// hasExpired
///
/// Description: returns true when the callback object has expired

          virtual bool hasExpired();
    protected:
        Sysreg_::ConfigParser & m_parser;
        string m_Cmd;
        long int m_Timeout;
        static string ROS_TIMER_TIMEOUT;
        static string ROS_TIMER_COMMAND;
    };

} /* EOF namspace System_ */


#endif /* EOF TIMER_COMMAND_CALLBACK_H__ */
