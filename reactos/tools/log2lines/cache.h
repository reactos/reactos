/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Image directory caching
 */

#ifndef __L2L_CACHE_H__
#define __L2L_CACHE_H__

int check_directory(int force);
int read_cache(void);
int create_cache(int force, int skipImageBase);

#endif /* __L2L_CACHE_H__ */

/* EOF */
