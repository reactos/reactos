
#ifndef __MODULE_H
#define __MODULE_H

#include <coff.h>

typedef struct
/*
 * 
 */
{   
        unsigned int text_base;
        unsigned int data_base;
        unsigned int bss_base;
        SCNHDR* scn_list;
        char* str_tab;
        SYMENT* sym_list;
        unsigned int size;

        /*
         * Base address of the module in memory
         */
        unsigned int base;

        /*
         * Offset of the raw data in memory
         */
        unsigned int raw_data_off;
} module;

int process_boot_module(unsigned int start);

#endif

