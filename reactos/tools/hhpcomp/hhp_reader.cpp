
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
#include <fstream>
#include <stdexcept>

#include <stdlib.h>

#include "hhp_reader.h"
#include "utils.h"

using namespace std;

string hhp_section::get_name()
{
    return name;
}

void hhp_section::set_name(string name)
{
    this->name = name;
}

hhp_pair::hhp_pair(string key, bool has_default_value, string default_value)
{
    this->key = key;
    this->has_default_value = has_default_value;
    this->default_value = default_value;
    value_has_been_set = false;
}

void hhp_pair::set_value(string value)
{
    this->value = value;
    value_has_been_set = true;
}

string hhp_pair::get_value()
{
    if (value_has_been_set)
        return value;
    else
    {
        if (has_default_value)
            return default_value;
        else
            throw domain_error("pair '" + key + "' does not have a default value");
    }
}

string hhp_pair::get_key()
{
    return key;
}

void hhp_key_value_section::process_line(string line)
{
    int pos_equals_sign = line.find_first_of('=');
    if (pos_equals_sign == string::npos)
        throw runtime_error("key-value pair does not contain an equals sign");
    string key = to_upper(line.substr(0, pos_equals_sign));
    string value = line.substr(pos_equals_sign + 1);
    if (key.length() == 0)
        throw runtime_error("key has length zero");

    entries.find(key)->second->set_value(value);
}

void hhp_key_value_section::add_entry(hhp_pair* entry)
{
    string upper_case_key = to_upper(entry->get_key());
    if (entries.count(upper_case_key) != 0)
        throw logic_error("trying to redundantly add key '" + upper_case_key + "'");
    entries.insert(pair<string, hhp_pair*>(upper_case_key, entry));
}

hhp_options_section::hhp_options_section()
{
    set_name("OPTIONS");

    add_entry(binary_TOC    = new hhp_pair("Binary TOC", true, "No"));
    add_entry(binary_index  = new hhp_pair("Binary Index", true, "Yes"));
    add_entry(compiled_file = new hhp_pair("Compiled File", false));
    add_entry(contents_file = new hhp_pair("Contents File", true, ""));
    add_entry(index_file    = new hhp_pair("Index File", true, ""));
    add_entry(autoindex     = new hhp_pair("AutoIndex", true, "No"));
    add_entry(defaultwindow = new hhp_pair("DefaultWindow", true, ""));//?
    add_entry(default_topic = new hhp_pair("Default Topic", true, "Index.htm"));//?
    add_entry(defaultfont   = new hhp_pair("DefaultFont", true, ""));
    add_entry(language      = new hhp_pair("Language", true, "0x409 English (US)"));//?
    add_entry(title         = new hhp_pair("Title", true, ""));//?
    add_entry(createchifile = new hhp_pair("CreateCHIFile", true, "No"));
    add_entry(compatibility = new hhp_pair("Compatibility", true, "1.1"));
    add_entry(errorlogfile  = new hhp_pair("ErrorLogFile", true, "Compiler.log"));//?
    add_entry(full_text_search = new hhp_pair("Full-text search", true, "Yes"));//?
    add_entry(display_compile_progress = new hhp_pair("Display compile progress", true, "Yes"));//?
    add_entry(display_compile_note = new hhp_pair("Display compile note", true, "Yes"));//?
    add_entry(flat          = new hhp_pair("Flat", true, "No"));
    add_entry(full_text_search_stop_list_file = new hhp_pair("Full text search stop list file", true, ""));
}

hhp_options_section::~hhp_options_section()
{
    delete binary_TOC;
    delete binary_index;
    delete compiled_file;
    delete contents_file;
    delete index_file;
    delete autoindex;
    delete defaultwindow;
    delete default_topic;
    delete defaultfont;
    delete language;
    delete title;
    delete createchifile;
    delete compatibility;
    delete errorlogfile;
    delete full_text_search;
    delete display_compile_progress;
    delete display_compile_note;
    delete flat;
    delete full_text_search_stop_list_file;
}

hhp_files_section::hhp_files_section()
{
    set_name("FILES");
}

void hhp_files_section::process_line(string line)
{
    filenames.push_back(line);
}

hhp_reader::hhp_reader(string filename)
{
    this->filename = filename;

    options = new hhp_options_section();
    add_section(options);
    files = new hhp_files_section();
    add_section(files);

    read();
    compute_unique_file_pathes_set();
}

hhp_reader::~hhp_reader()
{
    delete options;
    delete files;
}

void hhp_reader::add_section(hhp_section* section)
{
    string upper_case_name = to_upper(section->get_name());
    if (sections.count(upper_case_name) != 0)
        throw logic_error("trying to redundantly add section '" + upper_case_name + "'");
    sections.insert(pair<string, hhp_section*>(upper_case_name, section));
}

void hhp_reader::read()
{
    ifstream hhp_file;
    hhp_file.open(filename.c_str());

    string line;
    int line_number = 0;
    hhp_section* section = NULL;
    while (hhp_file.good())
    {
        getline(hhp_file, line);
        line_number++;
        if (line[line.length() - 1] == '\015')  // delete CR character if present
            line = line.substr(0, line.length() - 1);
        if (line[0] == '[' && line[line.length() - 1] == ']')
        {
            string name = to_upper(line.substr(1, line.length() - 2));
            if (sections.count(name))
            {
                section = sections.find(name)->second;
                clog << section->get_name() << endl;
            }
            else
            {
                clog << "unknown section: " << name << endl;
            }
        }
        else if (line[0] != ';' && !line.empty())
        {
            if (section)
                section->process_line(line);
        }
    }

    hhp_file.close();
}

void hhp_reader::compute_unique_file_pathes_set()
{
    for (list<string>::iterator it = files->filenames.begin(); it != files->filenames.end(); ++it)
    {
        unique_file_pathes.insert(replace_backslashes(realpath(it->c_str())));
    }
}

string hhp_reader::get_title_string()
{
    return options->title->get_value();
}

string hhp_reader::get_contents_file_string()
{
    return options->contents_file->get_value();
}

string hhp_reader::get_index_file_string()
{
    return options->index_file->get_value();
}

string hhp_reader::get_default_topic_string()
{
    return options->default_topic->get_value();
}

unsigned int hhp_reader::get_language_code()
{
    return strtoul(options->language->get_value().c_str(), NULL, 0);
}

string hhp_reader::get_compiled_file_string()
{
    return options->compiled_file->get_value();
}

set<string>::iterator hhp_reader::get_file_pathes_iterator_begin()
{
    return unique_file_pathes.begin();
}

set<string>::iterator hhp_reader::get_file_pathes_iterator_end()
{
    return unique_file_pathes.end();
}
