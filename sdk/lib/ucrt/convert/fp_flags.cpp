//
// fp_flags.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Public data used by the floating point library
//

extern "C" { int __fastflag{0}; }



// Routine to set the fast flag in order to speed up computation
// of transcendentals at the expense of limiting error checking
extern "C" int __cdecl __setfflag(int const new_flag)
{
    int const old_flag = __fastflag;
    __fastflag = new_flag;
    return old_flag;
}
