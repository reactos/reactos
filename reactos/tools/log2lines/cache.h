/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Image directory caching
 */

#pragma once

int check_directory(int force);
int read_cache(void);
int create_cache(int force, int skipImageBase);

/* EOF */
