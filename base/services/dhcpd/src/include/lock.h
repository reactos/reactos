#ifndef LOCK_H
#define LOCK_H

extern DHCPLIST *leased_list;

int check_leased_list();
static int test_and_set();
int lock_list();
int unlock_list();

#endif
