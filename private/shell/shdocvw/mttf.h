//
// MTTF messages
//
#define MTTF_STARTUP      40006
#define MTTF_SHUTDOWN     40007
#define MTTF_CRASHTRAPPED 40008

//
// Post a message to the MTTF window if it is present
// The MTTF tool tracks this process being up
//
void PostMTTFMessage(WPARAM mttfMsg);
