/*
 * Copyright 2018 Zebediah Figura
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

extern char PROG_FILES_DIR[MAX_PATH];
extern char PROG_FILES_DIR_NATIVE[MAX_PATH];
extern char COMMON_FILES_DIR[MAX_PATH];
extern char APP_DATA_DIR[MAX_PATH];
extern char WINDOWS_DIR[MAX_PATH];
extern char CURR_DIR[MAX_PATH];

BOOL get_system_dirs(void);
BOOL get_user_dirs(void);

typedef struct _msi_table
{
    const char *filename;
    const char *data;
    int size;
} msi_table;

#define ADD_TABLE(x) {#x".idt", x##_dat, sizeof(x##_dat)}

/* in install.c */
void create_database_wordcount(const char *name, const msi_table *tables, int num_tables,
    int version, int wordcount, const char *template, const char *packagecode);

#define create_database(name, tables, num_tables) \
    create_database_wordcount(name, tables, num_tables, 100, 0, ";1033", \
                              "{004757CA-5092-49C2-AD20-28E1CE0DF5F2}");

#define create_database_template(name, tables, num_tables, version, template) \
    create_database_wordcount(name, tables, num_tables, version, 0, template, \
                              "{004757CA-5092-49C2-AD20-28E1CE0DF5F2}");

void create_cab_file(const char *name, DWORD max_size, const char *files);
void create_file_data(const char *name, const char *data, DWORD size);
#define create_file(name, size) create_file_data(name, name, size)
void create_pf_data(const char *file, const char *data, BOOL is_file);
#define create_pf(file, is_file) create_pf_data(file, file, is_file)
void delete_cab_files(void);
BOOL delete_pf(const char *rel_path, BOOL is_file);
BOOL file_exists(const char *file);
BOOL pf_exists(const char *file);
BOOL is_process_limited(void);
UINT run_query(MSIHANDLE hdb, MSIHANDLE hrec, const char *query);
