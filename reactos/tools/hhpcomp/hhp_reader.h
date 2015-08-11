
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


#include <string>
#include <map>
#include <list>
#include <set>

using namespace std;  // using 'using' here for convenience

class hhp_reader;  // forward declaration

class hhp_section
{
private:
    string name;
    
public:
    virtual void process_line(string line) = 0;
    string get_name();
    void set_name(string name);
};

class hhp_pair
{
private:
    string key;
    bool value_has_been_set;
    string value;
    bool has_default_value;
    string default_value;
    
public:
    hhp_pair(string key, bool has_default_value = false, string default_value = "");
    void set_value(string value);
    string get_value();
    string get_key();
};

class hhp_key_value_section : public hhp_section
{
protected:
    map<string, hhp_pair*> entries;

    void add_entry(hhp_pair* entry);
    
public:
    virtual void process_line(string line);
};

class hhp_options_section : public hhp_key_value_section
{
    friend hhp_reader;

private:
    hhp_pair* binary_TOC;
    hhp_pair* binary_index;
    hhp_pair* compiled_file;
    hhp_pair* contents_file;
    hhp_pair* index_file;
    hhp_pair* autoindex;
    hhp_pair* defaultwindow;
    hhp_pair* default_topic;
    hhp_pair* defaultfont;
    hhp_pair* language;
    hhp_pair* title;
    hhp_pair* createchifile;
    hhp_pair* compatibility;
    hhp_pair* errorlogfile;
    hhp_pair* full_text_search;
    hhp_pair* display_compile_progress;
    hhp_pair* display_compile_note;
    hhp_pair* flat;
    hhp_pair* full_text_search_stop_list_file;
    
public:
    hhp_options_section();
    ~hhp_options_section();
};

class hhp_files_section : public hhp_section
{
    friend hhp_reader;

private:
    list<string> filenames;

public:
    hhp_files_section();
    virtual void process_line(string line);
};

class hhp_reader
{
private:
    string filename;
    map<string, hhp_section*> sections;
    hhp_options_section* options;
    hhp_files_section* files;
    set<string> unique_file_pathes;
    
    void add_section(hhp_section* section);
    void read();
    void compute_unique_file_pathes_set();
    
public:
    hhp_reader(string filename);
    ~hhp_reader();
    
    string get_title_string();
    string get_contents_file_string();
    string get_index_file_string();
    string get_default_topic_string();
    unsigned int get_language_code();
    string get_compiled_file_string();
    
    set<string>::iterator get_file_pathes_iterator_begin();
    set<string>::iterator get_file_pathes_iterator_end();
};

