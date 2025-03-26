
// This file is part of hhpcomp, a free HTML Help Project (*.hhp) compiler.
// Copyright (C) 2015  Benedikt Freisen
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


#include <iostream>
#include <string>
#include <set>
#include <stdexcept>

#include <sys/stat.h>

#include "hhp_reader.h"
#include "utils.h"

#if defined(_WIN32)
    #include <direct.h>
#else
    #include <unistd.h>
#endif

extern "C" {
#include "chmc/chmc.h"
#include "chmc/err.h"
}

extern "C" struct chmcTreeNode *chmc_add_file(struct chmcFile *chm, const char *filename,
                                   UInt16 prefixlen, int sect_id, UChar *buf,
                                   UInt64 len);


using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cerr << "Usage: hhpcomp <input.hhp>" << endl;
        exit(0);
    }

    string absolute_name = replace_backslashes(real_path(argv[1]));
    int prefixlen = absolute_name.find_last_of('/');
    clog << prefixlen << endl;
    if (chdir(absolute_name.substr(0, prefixlen).c_str()) == -1)  // change to the project file's directory
    {
        cerr << "chdir: working directory couldn't be changed" << endl;
        exit(0);
    }
    hhp_reader project_file(absolute_name);

    struct chmcFile chm;
    struct chmcConfig chm_config;

    memset(&chm, 0, sizeof(struct chmcFile));
    memset(&chm_config, 0, sizeof(struct chmcConfig));
    chm_config.title    = project_file.get_title_string().c_str();
    chm_config.hhc      = project_file.get_contents_file_string().c_str();
    chm_config.hhk      = project_file.get_index_file_string().c_str();
    chm_config.deftopic = project_file.get_default_topic_string().c_str();
    chm_config.language = project_file.get_language_code();
    chm_config.tmpdir   = ".";

    int err;
    err = chmc_init(&chm, replace_backslashes(project_file.get_compiled_file_string()).c_str(), &chm_config);
    if (err)
    {
        cerr << "could not initialize chmc" << endl;
        exit(EXIT_FAILURE);
    }

    for (set<string>::iterator it = project_file.get_file_pathes_iterator_begin();
         it != project_file.get_file_pathes_iterator_end(); ++it)
    {
        clog << "File: " << *it << endl;
        struct stat buf;
        stat(it->c_str(), &buf);
        if ((chmc_add_file(&chm, it->c_str(), prefixlen, 1, NULL, buf.st_size)) ? chmcerr_code() : CHMC_NOERR)
        {
            cerr << "could not add file: " << *it << endl;
            exit(EXIT_FAILURE);
        }
    }

    chmc_tree_done(&chm);
    chmc_term(&chm);

}
