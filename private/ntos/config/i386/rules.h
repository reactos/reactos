//
// Maximum data that can be specified (either as string or binary) in the 
// machine identification rules.
//

#define MAX_DESCRIPTION_LEN 256

BOOLEAN
CmpMatchInfList(
    IN PVOID InfImage,
    IN ULONG ImageSize,
    IN PCHAR Section
    );
