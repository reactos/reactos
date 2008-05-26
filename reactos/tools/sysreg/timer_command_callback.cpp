#include "timer_command_callback.h"


namespace System_
{
    using Sysreg_::ConfigParser;

    string TimerCommandCallback::ROS_TIMER_COMMAND="ROS_TIMER_COMMAND";
    string TimerCommandCallback::ROS_TIMER_TIMEOUT="ROS_TIMER_TIMEOUT";

//---------------------------------------------------------------
    TimerCommandCallback::TimerCommandCallback(ConfigParser &conf_parser) : TimerCallback(), m_parser(conf_parser), m_Cmd(""), m_Timeout(0)
    {
    }
//---------------------------------------------------------------
    TimerCommandCallback::~TimerCommandCallback()
    {
    }
//---------------------------------------------------------------
    bool TimerCommandCallback::initialize()
    {
        m_parser.getStringValue(ROS_TIMER_COMMAND, m_Cmd);
        m_parser.getIntValue(ROS_TIMER_TIMEOUT, m_Timeout);
        //FIXME
        // calculate current time 

        return true;
    }

//---------------------------------------------------------------
    bool TimerCommandCallback::hasExpired()
    {

        // FIXME
        // calculate current time
        // calculate difference
        // and invoke m_Cmd when it has expired
        return false;
    }

} // end of namespace System_
