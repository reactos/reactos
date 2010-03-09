#ifndef __GLUE_HEADER
#define __GLUE_HEADER

#include <stddef.h>

/* header for linux macros and definitions */

  /**
   * container_of - cast a member of a structure out to the containing structure
   * @ptr: the pointer to the member.
   * @type: the type of the container struct this is embedded in.
   * @member: the name of the member within the struct.
   *
   */
   #define container_of(ptr, type, member) (type *)( (char *)(ptr) - offsetof(type,member) )


#define time_after(a,b)         \
          ((long)(b) - (long)(a) < 0))

#define time_before(a,b)        time_after(b,a)

#define in_interrupt() ((__readeflags() >> 9) & 1)

typedef int (*acpi_table_handler) (ACPI_TABLE_HEADER *table);

typedef int (*acpi_table_entry_handler) (ACPI_SUBTABLE_HEADER *header, const unsigned long end);


#endif
