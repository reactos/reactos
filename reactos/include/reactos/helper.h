#ifndef _HELPER_H
#define _HELPER_H
 
#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))
#define ROUND_UP ROUNDUP
#define ROUND_DOWN ROUNDDOWN
#define PAGE_ROUND_DOWN(x) (((ULONG)x)&(~(PAGE_SIZE-1)))
#define PAGE_ROUND_UP(x) ( (((ULONG)x)%PAGE_SIZE) ? ((((ULONG)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG)x) )
#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))
#define RtlRosMin(X,Y) (((X) < (Y))? (X) : (Y))
#define RtlRosMin3(X,Y,Z) (((X) < (Y)) ? RtlRosMin(X,Z) : RtlRosMin(Y,Z))
#define KEBUGCHECKEX(a,b,c,d,e) DbgPrint("KeBugCheckEx at %s:%i\n",__FILE__,__LINE__), KeBugCheckEx(a,b,c,d,e)
#define KEBUGCHECK(a) DbgPrint("KeBugCheck at %s:%i\n",__FILE__,__LINE__), KeBugCheck(a)
#define EXPORTED __declspec(dllexport)
#define IMPORTED __declspec(dllimport)
#define LIST_FOR_EACH(entry, head) \
   for(entry = (head)->Flink; entry != (head); entry = entry->Flink)
#define LIST_FOR_EACH_SAFE(tmp_entry, head, ptr, type, field) \
   for ((tmp_entry)=(head)->Flink; (tmp_entry)!=(head) && \
        ((ptr) = CONTAINING_RECORD(tmp_entry,type,field)) && \
        ((tmp_entry) = (tmp_entry)->Flink); )
#define OPTHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 sizeof (IMAGE_NT_SIGNATURE)		     +	\
			 sizeof (IMAGE_FILE_HEADER)))
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))
#define NTSTAT_SEVERITY_SHIFT 30
#define NTSTAT_SEVERITY_MASK  0x00000003
#define NTSTAT_FACILITY_SHIFT 16
#define NTSTAT_FACILITY_MASK  0x00000FFF
#define NTSTAT_CUSTOMER_MASK  0x20000000
#define NT_SEVERITY(StatCode) (((StatCode) >> NTSTAT_SEVERITY_SHIFT) & NTSTAT_SEVERITY_MASK)
#define NT_FACILITY(StatCode) (((StatCode) >> NTSTAT_FACILITY_SHIFT) & NTSTAT_FACILITY_MASK)
#define NT_CUSTOMER(StatCode) ((StatCode) & NTSTAT_CUSTOMER_MASK)
#define RELATIVE_TIME(wait) (-(wait))
#define NANOS_TO_100NS(nanos) (((LONGLONG)(nanos)) / 100)
#define MICROS_TO_100NS(micros) (((LONGLONG)(micros)) * NANOS_TO_100NS(1000))
#define MILLIS_TO_100NS(milli) (((LONGLONG)(milli)) * MICROS_TO_100NS(1000))
#define SECONDS_TO_100NS(seconds) (((LONGLONG)(seconds)) * MILLIS_TO_100NS(1000))
#define MINUTES_TO_100NS(minutes) (((LONGLONG)(minutes)) * SECONDS_TO_100NS(60))
#define HOURS_TO_100NS(hours) (((LONGLONG)(hours)) * MINUTES_TO_100NS(60))
#define UNICODIZE1(x) L##x
#define UNICODIZE(x) UNICODIZE1(x)
#define InsertAscendingListFIFO(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField >\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}

#define InsertDescendingListFIFO(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField <\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}

#define InsertAscendingList(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField >=\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}

#define InsertDescendingList(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField <=\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}

#endif
