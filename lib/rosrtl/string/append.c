#define NTOS_MODE_USER
#include <ntos.h>

/* 
 * Utility to copy and append two unicode strings.
 *
 * IN OUT PUNICODE_STRING ResultFirst -> First string and result
 * IN     PUNICODE_STRING Second      -> Second string to append
 * IN     BOOL            Deallocate  -> TRUE: Deallocate First string before
 *                                       overwriting.
 *
 * Returns NTSTATUS.
 */

NTSTATUS NTAPI RosAppendUnicodeString(PUNICODE_STRING ResultFirst,
				      PUNICODE_STRING Second,
				      BOOL Deallocate) {
    NTSTATUS Status;
    PWSTR new_string = 
	RtlAllocateHeap(GetProcessHeap(),0,
			(ResultFirst->Length + Second->Length + sizeof(WCHAR)));
    if( !new_string ) {
	return STATUS_NO_MEMORY;
    }
    memcpy( new_string, ResultFirst->Buffer, 
	    ResultFirst->Length );
    memcpy( new_string + ResultFirst->Length / sizeof(WCHAR),
	    Second->Buffer,
	    Second->Length );
    if( Deallocate ) RtlFreeUnicodeString(ResultFirst);
    ResultFirst->Length += Second->Length;
    ResultFirst->MaximumLength = ResultFirst->Length;
    new_string[ResultFirst->Length / sizeof(WCHAR)] = 0;
    Status = RtlCreateUnicodeString(ResultFirst,new_string) ? 
	STATUS_SUCCESS : STATUS_NO_MEMORY;
    RtlFreeHeap(GetProcessHeap(),0,new_string);
    return Status;
}
