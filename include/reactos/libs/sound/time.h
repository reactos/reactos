/*
    ReactOS Sound System
    Timing helper

    Author:
        Andrew Greenwood (andrew.greenwood@silverblade.co.uk)

    History:
        1 July 2008 - Created
*/

#ifndef ROS_SOUND_TIME_H
#define ROS_SOUND_TIME_H

VOID
SleepMs(ULONG Milliseconds);

ULONG
QuerySystemTimeMs();

#endif
