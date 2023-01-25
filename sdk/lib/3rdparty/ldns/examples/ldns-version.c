/*
 * ldns-version shows ldns's version 
 *
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

#include "config.h"
#include <ldns/ldns.h>

int
main(void)
{
	printf("%s\n", ldns_version());
        return 0;
}
