/* SYSTEM STRUCTURES ******************************************************/

#include <internal/hal/hal.h>
#include <ddk/cfgtypes.h>
#include <ddk/ketypes.h>
#include <ddk/obtypes.h>
#include <ddk/mmtypes.h>
#include <ddk/iotypes.h>
#include <ddk/extypes.h>
#include <ddk/pstypes.h>

/*
 * PURPOSE: Thread object
 */
typedef struct 
{
   CSHORT Type;
   CSHORT Size;
   
   /*
    * PURPOSE: Entry in the linked list of threads
    */
   LIST_ENTRY Entry;
   
   /*
    * PURPOSE: Current state of the thread
    */
   ULONG State;
   
   /*
    * PURPOSE: Priority modifier of the thread
    */
   ULONG Priority;
   
   /*
    * PURPOSE: Pointer to our parent process
    */
//   PEPROCESS Parent;
   
   /*
    * PURPOSE: Handle of our parent process
    */
   HANDLE ParentHandle;
   
   /*
    * PURPOSE: Not currently used 
    */
   ULONG AffinityMask;
   
   /*
    * PURPOSE: Saved thread context
    */
   hal_thread_state context;
   
} THREAD_OBJECT, *PTHREAD_OBJECT;



/*
 * PURPOSE: Object describing the wait a thread is currently performing
 */
typedef struct
{
   /*
    * PURPOSE: Pointer to the waiting thread
    */
   PTHREAD_OBJECT thread;
   
   /*
    * PURPOSE: Entry in the wait queue for the object being waited on
    */
   LIST_ENTRY Entry;
   
   /*
    * PURPOSE: Pointer to the object being waited on
    */
   DISPATCHER_HEADER* wait_object;
   
} KWAIT_BLOCK, *PKWAIT_BLOCK;
   
typedef struct _ADAPTER_OBJECT
{
} ADAPTER_OBJECT, *PADAPTER_OBJECT;

typedef struct _CONTROLLER_OBJECT
{
   PVOID ControllerExtension;   
} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;
   
typedef struct _STRING
{
   /*
    * Length in bytes of the string stored in buffer
    */
   USHORT Length;
   
   /*
    * Maximum length of the string 
    */
   USHORT MaximumLength;
   
   /*
    * String
    */
   PCHAR Buffer;
} STRING, *PSTRING;

typedef struct _ANSI_STRING
{
   /*
    * Length in bytes of the string stored in buffer
    */
   USHORT Length;
   
   /*
    * Maximum length of the string 
    */
   USHORT MaximumLength;
   
   /*
    * String
    */
   PCHAR Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _KTIMER
{
   /*
    * Pointers to maintain the linked list of activated timers
    */
   LIST_ENTRY entry;
   
   /*
    * Absolute expiration time in system time units
    */
   unsigned long long expire_time;

   /*
    * Optional dpc associated with the timer 
    */
   PKDPC dpc;
   
   /*
    * True if the timer is signaled
    */
   BOOLEAN signaled;
   
   /*
    * True if the timer is in the system timer queue
    */
   BOOLEAN running;
   
   /*
    * Type of the timer either Notification or Synchronization
    */
   TIMER_TYPE type;
   
   /*
    * Period of the timer in milliseconds (zero if once-only)
    */
   ULONG period;
   
} KTIMER, *PKTIMER;




typedef struct _IO_RESOURCE_DESCRIPTOR
{
   UCHAR Option;
   UCHAR Type;
   UCHAR SharedDisposition;
   
   /*
    * Reserved for system use
    */
   UCHAR Spare1;             
   
   USHORT Flags;
   
   /*
    * Reserved for system use
    */
   UCHAR Spare2;
   
   union
     {
	struct
	  {
	     ULONG Length;
	     ULONG Alignment;
	     PHYSICAL_ADDRESS MinimumAddress;
	     PHYSICAL_ADDRESS MaximumAddress;
	  } Port;
	struct
	  {
	     ULONG Length;
	     ULONG Alignment;
	     PHYSICAL_ADDRESS MinimumAddress;
	     PHYSICAL_ADDRESS MaximumAddress;
	  } Memory;
	struct
	  { 
	     ULONG MinimumVector;
	     ULONG MaximumVector;
	  } Interrupt;
	struct
	  {
	     ULONG MinimumChannel;
	     ULONG MaximumChannel;
	  } Dma;
     } u;     
} IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

typedef struct _IO_RESOURCE_LIST
{
   USHORT Version;
   USHORT Revision;
   ULONG Count;
   IO_RESOURCE_DESCRIPTOR Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCES_REQUIREMENTS_LIST
{
   /*
    * List size in bytes
    */
   ULONG ListSize;
   
   /*
    * System defined enum for the bus
    */
   INTERFACE_TYPE InterfaceType;
   
   ULONG BusNumber;
   ULONG SlotNumber;
   ULONG Reserved[3];
   ULONG AlternativeLists;
   IO_RESOURCE_LIST List[1];   
} IO_RESOURCES_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;

typedef struct
{
   UCHAR Type;
   UCHAR ShareDisposition;
   USHORT Flags;
   union
     {
	struct
	  {
	     PHYSICAL_ADDRESS Start;
	     ULONG Length;
	  } Port;
	struct
	  {
	     ULONG Level;
	     ULONG Vector;
	     ULONG Affinity;
	  } Interrupt;
	struct
	  {
	     PHYSICAL_ADDRESS Start;
	     ULONG Length;
	  } Memory;
	struct
	  {
	     ULONG Channel;
	     ULONG Port;
	     ULONG Reserved1;
	  } Dma;
	struct
	  {
	     ULONG DataSize;
	     ULONG Reserved1;
	     ULONG Reserved2;
	  } DeviceSpecificData;
     } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

typedef struct
{
   USHORT Version;
   USHORT Revision;
   ULONG Count;
   CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST;

typedef struct
{
   INTERFACE_TYPE InterfaceType;
   ULONG BusNumber;
   CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR;

typedef struct
{
   ULONG Count;
   CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

struct _KINTERRUPT;

typedef BOOLEAN (*PKSERVICE_ROUTINE)(struct _KINTERRUPT* Interrupt, 
			     PVOID ServiceContext);

typedef struct _KINTERRUPT
{
   ULONG Vector;
   KAFFINITY ProcessorEnableMask;
   PKSPIN_LOCK IrqLock;
   BOOLEAN Shareable;
   BOOLEAN FloatingSave;
   PKSERVICE_ROUTINE ServiceRoutine;
   PVOID ServiceContext;
   LIST_ENTRY Entry;
   KIRQL SynchLevel;
} KINTERRUPT, *PKINTERRUPT;

