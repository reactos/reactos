#ifndef LEASES_H
#define LEASES_H

extern DHCPLIST *list;
int find_lease( DHCPLEASE *, u32b, u8b * );
int init_leases_list();
int confirm_lease( DHCPLEASE *, u32b );
int release_lease( DHCPLEASE *, u32b, u8b * );

#endif
