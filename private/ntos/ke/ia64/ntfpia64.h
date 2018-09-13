
//
// Floating Point State Structure
//

typedef struct _FLOATING_POINT_STATE {
    PVOID TrapFrame;
    PVOID ExceptionFrame;
} FLOATING_POINT_STATE, *PFLOATING_POINT_STATE;

