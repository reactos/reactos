#ifndef TIMER_CALLBACK_H__
#define TIMER_CALLBACK_H__


namespace System_
{
//---------------------------------------------------------------------------------------
///
/// TimerCallback
///
/// Description: base class for all timer callback functions

    class TimerCallback
    {
	public:
//---------------------------------------------------------------------------------------
///
/// TimerCallback
///
/// Description: constructor of class TimerCallback

    TimerCallback() {}

//---------------------------------------------------------------------------------------
///
/// TimerCallback
///
/// Description: destructor of class TimerCallback

    virtual ~TimerCallback() {}


//---------------------------------------------------------------------------------------
///
/// initialize
///
/// Description: this function is called when the timercallback object is initialized in
/// in the callback mechanism

    virtual bool initialize(){ return true;}


//---------------------------------------------------------------------------------------
///
/// hasExpired
///
/// Description: this function is called everytime timer event has started. The callback object
/// should return true if the expiration function has expired.
/// Note: the function should then also cleanup all consumed resourced

    virtual bool hasExpired() = 0;
    }; //end of class TimerCallback

}// end of namespace System_

#endif
