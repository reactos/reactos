#include "ksia64.h"

//--------------------------------------------------------------------
// Routine:
//
//       VOID
//       KiSaveEmDebugContext(
//            IN  OUT PCONTEXT  Context)
//
// Description:
//
//       This function takes the contents of the EM debug registers 
//       and saves them in the specified EM mode Context frame.
//
// Input:
//
//       a0: Context - Pointer to the EM Context frame where the debug 
//                     registers should be saved.
//
// Output:
//
//       Stores the debug registers in the target EM Context frame.
//
// Return value:
//
//       None
//
//
// N.B. The format iA mode FP registers is 80 bits and will not change.
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiSaveEmDebugContext)

        ARGPTR      (a0)

        add         t1 = CxDbD0, a0              // Point at Context's DbD0
        add         t2 = CxDbI0, a0              // Point at Context's DbI0

        mov         t5 = dbr[r0]                 // Get Dbr0         
        mov         t6 = ibr[r0]                 // Get Ibr0         
        ;;

        st8         [t1] = t5, CxDbD1 - CxDbD0   // Save Dbr
        st8         [t2] = t6, CxDbI1 - CxDbI0   // Save Ibr

        add         t3 = 1, r0                   // Next is Dbr1
        add         t4 = 1, r0                   // Next is Ibr1
        ;;
         
        mov         t5 = dbr[t3]                 // Get Dbr1         
        mov         t6 = ibr[t4]                 // Get Ibr1         
        ;;

        st8         [t1] = t5, CxDbD2 - CxDbD1   // Save Dbr
        st8         [t2] = t6, CxDbI2 - CxDbI1   // Save Ibr

        add         t3 = 1, t3                   // Next is Dbr2
        add         t4 = 1, t4                   // Next is Ibr2
        ;;
         
        mov         t5 = dbr[t3]                 // Get Dbr2         
        mov         t6 = ibr[t4]                 // Get Ibr2         
        ;;

        st8         [t1] = t5, CxDbD3 - CxDbD2   // Save Dbr
        st8         [t2] = t6, CxDbI3 - CxDbI2   // Save Ibr

        add         t3 = 1, t3                   // Next is Dbr3
        add         t4 = 1, t4                   // Next is Ibr3
        ;;
         
        mov         t5 = dbr[t3]                 // Get Dbr3         
        mov         t6 = ibr[t4]                 // Get Ibr3         
        ;;

        st8         [t1] = t5, CxDbD4 - CxDbD3   // Save Dbr
        st8         [t2] = t6, CxDbI4 - CxDbI3   // Save Ibr

        add         t3 = 1, t3                   // Next is Dbr4
        add         t4 = 1, t4                   // Next is Ibr4
        ;;
         
        mov         t5 = dbr[t3]                 // Get Dbr4         
        mov         t6 = ibr[t4]                 // Get Ibr4         
        ;;

        st8         [t1] = t5, CxDbD5 - CxDbD4   // Save Dbr
        st8         [t2] = t6, CxDbI5 - CxDbI4   // Save Ibr

        add         t3 = 1, t3                   // Next is Dbr5
        add         t4 = 1, t4                   // Next is Ibr5
        ;;
         
        mov         t5 = dbr[t3]                 // Get Dbr5         
        mov         t6 = ibr[t4]                 // Get Ibr5         
        ;;

        st8         [t1] = t5, CxDbD6 - CxDbD5   // Save Dbr
        st8         [t2] = t6, CxDbI6 - CxDbI5   // Save Ibr

        add         t3 = 1, t3                   // Next is Dbr6
        add         t4 = 1, t4                   // Next is Ibr6
        ;;
         
        mov         t5 = dbr[t3]                 // Get Dbr6         
        mov         t6 = ibr[t4]                 // Get Ibr6         
        ;;

        st8         [t1] = t5, CxDbD7 - CxDbD6   // Save Dbr
        st8         [t2] = t6, CxDbI7 - CxDbI6   // Save Ibr
                                                    
        add         t3 = 1, t3                   // Next is Dbr7
        add         t4 = 1, t4                   // Next is Ibr7
        ;;
         
        mov         t5 = dbr[t3]                 // Get Dbr7         
        mov         t6 = ibr[t4]                 // Get Ibr7         
        ;;

        st8         [t1] = t5, CxFltS0 - CxDbD7  // Save Dbr
        st8         [t2] = t6, CxDbD0  - CxDbI7  // Save Ibr
        br.ret.sptk brp
        ;;

        LEAF_EXIT(KiSaveEmDebugContext)   

//--------------------------------------------------------------------
// Routine:
//
//       VOID
//       KiLoadEmDebugContext(
//          IN  PCONTEXT  Context)
//
// Description:
//
//       This function takes the values stored for the EM debug registers
//       in the specified EM mode Context frame and loads the debug registers
//       for the thread with them.
//
// Input:
//
//       a0: Context - Pointer to the EM Context frame where the debug 
//                     register contents are to be loaded from.
//
// Output:
//
//       None
//
// Return value:
//
//       None
//
// N.B. The format iA mode FP registers is 80 bits and will not change.
//
//--------------------------------------------------------------------
        LEAF_ENTRY(KiLoadEmDebugContext)

        ARGPTR      (a0)

        add         t1 = CxDbD0, a0              // Point at Context's DbD0
        add         t2 = CxDbI0, a0              // Point at Context's DbI0
        ;;

        ld8         t5 = [t1], CxDbD1 - CxDbD0   // Load Value for Dbr0 
        ld8         t6 = [t2], CxDbI1 - CxDbI0   // Load Value for Ibr0
         
        ;;
        mov         dbr[r0] = t5                 // Load Dbr0            
        mov         ibr[r0] = t6                 // Load Ibr0            

        add         t3 = 1, r0                   // Start with Dbr1
        add         t4 = 1, r0                   // Start with Ibr1

        ld8         t5 = [t1], CxDbD2 - CxDbD1   // Load Value for Dbr1 
        ld8         t6 = [t2], CxDbI2 - CxDbI1   // Load Value for Ibr1
        ;;
         
        mov         dbr[t3] = t5                 // Load Dbr1            
        mov         ibr[t4] = t6                 // Load Ibr1            

        add         t3 = 2, r0                   // Start with Dbr2
        add         t4 = 2, r0                   // Start with Ibr2

        ld8         t5 = [t1], CxDbD3 - CxDbD2   // Load Value for Dbr2 
        ld8         t6 = [t2], CxDbI3 - CxDbI2   // Load Value for Ibr2
        ;;
         
        mov         dbr[t3] = t5                 // Load Dbr2            
        mov         ibr[t4] = t6                 // Load Ibr2            

        add         t3 = 3, r0                   // Start with Dbr3
        add         t4 = 3, r0                   // Start with Ibr3

        ld8         t5 = [t1], CxDbD4 - CxDbD3   // Load Value for Dbr3 
        ld8         t6 = [t2], CxDbI4 - CxDbI3   // Load Value for Ibr3
        ;;
         
        mov         dbr[t3] = t5                 // Load Dbr3            
        mov         ibr[t4] = t6                 // Load Ibr3            

        add         t3 = 4, r0                   // Start with Dbr4
        add         t4 = 4, r0                   // Start with Ibr4

        ld8         t5 = [t1], CxDbD5 - CxDbD4   // Load Value for Dbr4 
        ld8         t6 = [t2], CxDbI5 - CxDbI4   // Load Value for Ibr4
        ;;
         
        mov         dbr[t3] = t5                 // Load Dbr4            
        mov         ibr[t4] = t6                 // Load Ibr4            

        add         t3 = 5, r0                   // Start with Dbr5
        add         t4 = 5, r0                   // Start with Ibr5

        ld8         t5 = [t1], CxDbD6 - CxDbD5   // Load Value for Dbr5 
        ld8         t6 = [t2], CxDbI6 - CxDbI5   // Load Value for Ibr5
        ;;
         
        mov         dbr[t3] = t5                 // Load Dbr5            
        mov         ibr[t4] = t6                 // Load Ibr5            
                                                                 
        add         t3 = 6, r0                   // Start with Dbr6
        add         t4 = 6, r0                   // Start with Ibr6

        ld8         t5 = [t1], CxDbD7 - CxDbD6   // Load Value for Dbr6 
        ld8         t6 = [t2], CxDbI7 - CxDbI6   // Load Value for Ibr6
        ;;
         
        mov         dbr[t3] = t5                 // Load Dbr6            
        mov         ibr[t4] = t6                 // Load Ibr6            
                                                                 
        add         t3 = 7, r0                   // Start with Dbr7
        add         t4 = 7, r0                   // Start with Ibr7

        ld8         t5 = [t1], CxFltS0- CxDbD7   // Load Value for Dbr7 
        ld8         t6 = [t2], CxDbD0 - CxDbI7   // Load Value for Ibr7
        ;;
         
        mov         dbr[t3] = t5                 // Load Dbr7            
        mov         ibr[t4] = t6                 // Load Ibr7            
        br.ret.sptk brp

        LEAF_EXIT(KiLoadEmDebugContext)
