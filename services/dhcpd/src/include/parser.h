#ifndef PARSER_H
#define PARSER_H

int parse_dhcp_options( DHCPMESSAGE *, DHCPOPTIONS *);
int process_dhcp_packet( DHCPMESSAGE *, DHCPOPTIONS *);
int write_packet( DHCPMESSAGE *, char *);

#endif
