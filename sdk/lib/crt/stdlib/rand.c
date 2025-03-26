/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>
#include <ntsecapi.h>
#include <internal/tls.h>

/*
 * @implemented
 */
int
rand(void)
{
    thread_data_t *data = msvcrt_get_thread_data();

    /* this is the algorithm used by MSVC, according to
     * http://en.wikipedia.org/wiki/List_of_pseudorandom_number_generators */
    data->random_seed = data->random_seed * 214013 + 2531011;
    return (data->random_seed >> 16) & RAND_MAX;
}

/*
 * @implemented
 */
void
srand(unsigned int seed)
{
    thread_data_t *data = msvcrt_get_thread_data();
    data->random_seed = seed;
}
