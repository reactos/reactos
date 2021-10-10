/*
 * PROJECT:     ReactOS TXT to NLS Converter
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later.html)
 * FILE:        sdk/tools/txt2nls/main.c
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <iostream>
#include <fstream>
#include <limits>
#include <vector>
#include <cstring>
#include <string>

static const char whitespaces[] = " \t\f\v\n\r";
static long line_number = -1;

#pragma pack(push, 1)
#define MAXIMUM_LEADBYTES 12
struct NLS_FILE_HEADER
{
    uint16_t HeaderSize;
    uint16_t CodePage;
    uint16_t MaximumCharacterSize;
    uint16_t DefaultChar;
    uint16_t UniDefaultChar;
    uint16_t TransDefaultChar;
    uint16_t TransUniDefaultChar;
    uint8_t LeadByte[MAXIMUM_LEADBYTES];
};
static_assert(sizeof(NLS_FILE_HEADER) == 26, "Wrong size for NLS_FILE_HEADER");
#pragma pack(pop)

static std::istream& get_clean_line(std::istream& stream, std::string& str)
{
    do
    {
        std::istream& ret = std::getline(stream, str);
        if (!ret)
            return ret;

        /* Ignore comments */
        std::size_t comment_pos = str.find_first_of(';');
        if (comment_pos != std::string::npos)
        {
            str.erase(comment_pos);
        }

        /* Remove trailing spaces */
        std::size_t end_of_line = str.find_last_not_of(whitespaces);
        if (end_of_line != std::string::npos)
            str.erase(end_of_line + 1);
        else
            str.clear();

        line_number++;
    } while (str.empty());

    return stream;
}

static void tokenize(std::string& str, std::string& token)
{
    std::size_t token_start = str.find_first_not_of(whitespaces);
    if (token_start == std::string::npos)
    {
        token = "";
        str.clear();
        return;
    }

    std::size_t token_end = str.find_first_of(whitespaces, token_start);
    if (token_end == std::string::npos)
    {
        token = str.substr(token_start);
        str.clear();
        return;
    }

    token = str.substr(token_start, token_end);
    str.erase(0, str.find_first_not_of(whitespaces, token_end));
}

template<typename T>
static void tokenize(std::string& str, T& int_token, int base = 0)
{
    std::string token;
    tokenize(str, token);

    long val;
    val = std::stol(token, nullptr, base);
    if ((val > std::numeric_limits<T>::max()) || (val < std::numeric_limits<T>::min()))
        throw std::invalid_argument(token + " does not fit range ["
            + std::to_string(std::numeric_limits<T>::min()) + ":" + std::to_string(std::numeric_limits<T>::max()) + "]");

    int_token = val;
}

void error(const std::string& err)
{
    std::cerr << "Error parsing line " << line_number <<": " << err << std::endl;
    std::exit(1);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <txt_in> <nls_out>" << std::endl;
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input.is_open())
    {
        std::cerr << "Unable to open " << argv[1] << std::endl;
        return 1;
    }

    NLS_FILE_HEADER FileHeader;
    memset(&FileHeader, 0, sizeof(FileHeader));

    std::string curr_line;
    // Get code page
    if (!get_clean_line(input, curr_line))
    {
        std::cerr << "ERROR: File is empty" << std::endl;
        return 1;
    }

    std::string token;
    tokenize(curr_line, token);
    if (token != "CODEPAGE")
        error("expected CODEPAGE, got \"" + token + "\" instead");
    try
    {
        tokenize(curr_line, FileHeader.CodePage, 10);
    }
    catch(const std::invalid_argument& ia)
    {
        error(ia.what());
    }

    if (!curr_line.empty())
        error("Garbage after CODEPAGE statement: \"" + curr_line + "\"");

    /* Get CPINFO */
    if (!get_clean_line(input, curr_line))
        error("Nothing after CODEPAGE statement");

    tokenize(curr_line, token);
    if (token != "CPINFO")
        error("Expected CPINFO, got \"" + token + "\" instead");
    try
    {
        tokenize(curr_line, FileHeader.MaximumCharacterSize);
        tokenize(curr_line, FileHeader.DefaultChar);
        tokenize(curr_line, FileHeader.UniDefaultChar);
    }
    catch(const std::invalid_argument& ia)
    {
        error(ia.what());
        return 1;
    }
    if (!curr_line.empty())
        error("Garbage after CPINFO statement: \"" + curr_line + "\"");
    if ((FileHeader.MaximumCharacterSize != 1) && (FileHeader.MaximumCharacterSize != 2))
        error("Expected 1 or 2 as max char size in CPINFO, got \"" + std::to_string(FileHeader.MaximumCharacterSize) + "\" instead");
    if ((FileHeader.MaximumCharacterSize == 1) && (FileHeader.DefaultChar > std::numeric_limits<uint8_t>::max()))
        error("Default MB character " + std::to_string(FileHeader.DefaultChar) + " doesn't fit in a 8-bit value");

    /* Setup tables & default values */
    bool has_mbtable = false;
    uint16_t mb_table[256] = {0};

    bool has_wctable = false;
    uint8_t* wc_table = new uint8_t[65536 * FileHeader.MaximumCharacterSize];
    if (FileHeader.MaximumCharacterSize == 1)
    {
        for (int i = 0; i < 65536; i++)
            wc_table[i] = FileHeader.DefaultChar;
    }
    else
    {
        uint16_t* wc_table_dbcs = reinterpret_cast<uint16_t*>(wc_table);
        for (int i = 0; i < 65536; i++)
            wc_table_dbcs[i] = FileHeader.DefaultChar;
    }

    std::vector<uint16_t> dbcs_table;
    uint16_t lb_offsets[256] = {0};
    uint16_t dbcs_range_count = 0;

    uint16_t glyph_table[256] = {0};
    bool has_glyphs = false;

    /* Now parse */
    while (get_clean_line(input, curr_line))
    {
        tokenize(curr_line, token);

        if (token == "ENDCODEPAGE")
        {
            if (!curr_line.empty())
                error("Garbage after ENDCODEPAGE statement: \"" + curr_line + "\"");
            break;
        }
        else if (token == "MBTABLE")
        {
            uint16_t table_size;
            try
            {
                tokenize(curr_line, table_size);
            }
            catch(const std::invalid_argument& ia)
            {
                error(ia.what());
            }
            if (has_mbtable)
                error("MBTABLE can only be declared once");
            if (table_size > 256)
                error("MBTABLE size can't be larger than 256");
            if (!curr_line.empty())
                error("Garbage after MBTABLE statement: \"" + curr_line + "\"");

            has_mbtable = true;
            while (table_size--)
            {
                if (!get_clean_line(input, curr_line))
                    error("Expected " + std::to_string(table_size + 1) + " more lines after MBTABLE token");

                uint8_t mb;
                uint16_t wc;

                try
                {
                    tokenize(curr_line, mb);
                    tokenize(curr_line, wc);
                }
                catch(const std::invalid_argument& ia)
                {
                    error(ia.what());
                }
                if (!curr_line.empty())
                    error("Garbage after MBTABLE entry: \"" + curr_line + "\"");
                mb_table[mb] = wc;
            }
        }
        else if (token == "WCTABLE")
        {
            uint32_t table_size;
            try
            {
                tokenize(curr_line, table_size);
            }
            catch(const std::invalid_argument& ia)
            {
                error(ia.what());
            }
            if (has_wctable)
                error("WCTABLE can only be declared once");
            if (!curr_line.empty())
                error("Garbage after WCTABLE statement: \"" + curr_line + "\"");
            if (table_size > 65536)
                error("WCTABLE size can't be larger than 65536");

            has_wctable = true;

            if (FileHeader.MaximumCharacterSize == 1)
            {
                while (table_size--)
                {
                    if (!get_clean_line(input, curr_line))
                        error("Expected " + std::to_string(table_size + 1) + " more lines after WCTABLE token");

                    uint8_t mb;
                    uint16_t wc;

                    try
                    {
                        tokenize(curr_line, wc);
                        tokenize(curr_line, mb);
                    }
                    catch(const std::invalid_argument& ia)
                    {
                        error(ia.what());
                    }
                    if (!curr_line.empty())
                        error("Garbage after WCTABLE entry: \"" + curr_line + "\"");
                    wc_table[wc] = mb;
                }
            }
            else
            {
                uint16_t* wc_table_dbcs = reinterpret_cast<uint16_t*>(wc_table);
                while (table_size--)
                {
                    if (!get_clean_line(input, curr_line))
                        error("Expected " + std::to_string(table_size + 1) + " more lines after WCTABLE token");
                    uint16_t mb;
                    uint16_t wc;

                    try
                    {
                        tokenize(curr_line, wc);
                        tokenize(curr_line, mb);
                    }
                    catch(const std::invalid_argument& ia)
                    {
                        error(ia.what());
                    }
                    if (!curr_line.empty())
                        error("Garbage after MBTABLE entry: \"" + curr_line + "\"");
                    wc_table_dbcs[wc] = mb;
                }
            }
        }
        else if (token == "DBCSRANGE")
        {
            if (dbcs_range_count != 0)
                error("DBCSRANGE can only be declared once");

            try
            {
                tokenize(curr_line, dbcs_range_count);
            }
            catch(const std::invalid_argument& ia)
            {
                error(ia.what());
            }
            if (dbcs_range_count > (MAXIMUM_LEADBYTES / 2))
                error("DBCSRANGE count can't exceed " + std::to_string(MAXIMUM_LEADBYTES / 2));
            if (!curr_line.empty())
                error("Garbage after DBCSRANGE token");

            std::size_t current_offset = 0;

            uint16_t range_count = dbcs_range_count;
            uint16_t current_range = 0;
            while (range_count--)
            {
                if (!get_clean_line(input, curr_line))
                    error("Expected new range after DBCSRANGE");

                uint8_t RangeStart, RangeEnd;
                try
                {
                    tokenize(curr_line, RangeStart);
                    tokenize(curr_line, RangeEnd);
                }
                catch(const std::invalid_argument& ia)
                {
                    error(ia.what());
                }
                if (!curr_line.empty())
                    error("Garbage after DBCS range declaration");

                if (RangeStart > RangeEnd)
                    error("Invalid range specified for DBCSRANGE");

                FileHeader.LeadByte[current_range*2] = RangeStart;
                FileHeader.LeadByte[current_range*2+1] = RangeEnd;
                current_range++;

                dbcs_table.resize(dbcs_table.size() + 256 * (RangeEnd - RangeStart + 1), FileHeader.UniDefaultChar);

                for (uint8_t LeadByte = RangeStart; LeadByte <= RangeEnd; LeadByte++)
                {
                    if (!get_clean_line(input, curr_line))
                        error("Expected new DBCSTABLE after DBCS range declaration");

                    tokenize(curr_line, token);
                    if (token != "DBCSTABLE")
                        error("Expected new DBCSTABLE after DBCS range declaration");

                    uint16_t table_size;
                    try
                    {
                        tokenize(curr_line, table_size);
                    }
                    catch(const std::invalid_argument& ia)
                    {
                        error(ia.what());
                    }
                    if (table_size > 256)
                        error("DBCSTABLE can't have more than 256 entries");
                    while (table_size--)
                    {
                        if (!get_clean_line(input, curr_line))
                            error("Expected " + std::to_string(table_size + 1) + " more lines after DBCSTABLE token");

                        uint8_t mb;
                        uint16_t wc;

                        try
                        {
                            tokenize(curr_line, mb);
                            tokenize(curr_line, wc);
                        }
                        catch(const std::invalid_argument& ia)
                        {
                            error(ia.what());
                        }
                        if (!curr_line.empty())
                            error("Garbage after DBCSTABLE entry: \"" + curr_line + "\"");

                        dbcs_table[current_offset + mb] = wc;
                    }
                    current_offset += 256;
                    /* Offsets start at 256 for the offset table. */
                    lb_offsets[LeadByte] = current_offset;
                }
            }
        }
        else if (token == "GLYPHTABLE")
        {
            uint16_t table_size;
            try
            {
                tokenize(curr_line, table_size);
            }
            catch(const std::invalid_argument& ia)
            {
                error(ia.what());
            }
            if (has_glyphs)
                error("GLYPHTABLE can only be declared once");
            if (table_size > 256)
                error("GLYPHTABLE size can't be larger than 256");
            if (!curr_line.empty())
                error("Garbage after GLYPHTABLE statement: \"" + curr_line + "\"");
            has_glyphs = true;

            while (table_size--)
            {
                if (!get_clean_line(input, curr_line))
                    error("Expected " + std::to_string(table_size + 1) + " more lines after GLYPHTABLE token");

                uint8_t mb;
                uint16_t wc;

                try
                {
                    tokenize(curr_line, mb);
                    tokenize(curr_line, wc);
                }
                catch(const std::invalid_argument& ia)
                {
                    error(ia.what());
                }
                if (!curr_line.empty())
                    error("Garbage after GLYPHTABLE entry: \"" + curr_line + "\"");
                glyph_table[mb] = wc;
            }
        }
        else
        {
            error("Unexpected token \"" + token + "\"");
        }
    }

    if (token != "ENDCODEPAGE")
        error("Expected last token to be \"ENDCODEPAGE\"");

    input.close();

    /* Ensure this is minimally workable */
    if (!has_mbtable)
        error("File has no MBTABLE statement");
    if (!has_wctable)
        error("File has no WCTABLE statement");

    /* Glyph table fixup */
    if (has_glyphs)
    {
        for(int i = 0; i < 256; i++)
        {
            if (glyph_table[i] == 0)
                glyph_table[i] = mb_table[i];
        }
    }

    /* Translated default char fixup */
    if (FileHeader.MaximumCharacterSize == 1)
    {
        FileHeader.TransDefaultChar = mb_table[FileHeader.DefaultChar];
        FileHeader.TransUniDefaultChar = wc_table[FileHeader.UniDefaultChar];
    }
    else
    {
        if (FileHeader.DefaultChar > 0xFF)
        {
            uint16_t offset = lb_offsets[FileHeader.DefaultChar >> 8];
            if (!offset)
                error("Default MB char is not translatable!");
            FileHeader.TransDefaultChar = dbcs_table[(FileHeader.DefaultChar & 0xFF) + (offset - 256)];
        }
        else
        {
            FileHeader.TransDefaultChar = mb_table[FileHeader.DefaultChar];
        }
        uint16_t* wc_table_dbcs = reinterpret_cast<uint16_t*>(wc_table);
        FileHeader.TransUniDefaultChar = wc_table_dbcs[FileHeader.UniDefaultChar];
    }
    FileHeader.HeaderSize = sizeof(NLS_FILE_HEADER) / sizeof(uint16_t);

    std::ofstream output(argv[2], std::ios_base::binary);

    output.write(reinterpret_cast<char*>(&FileHeader), sizeof(FileHeader));

    uint16_t wc_table_offset = sizeof(mb_table) / sizeof(uint16_t)
                               + 1                                  /* size of glyph table */
                               + (has_glyphs ? 256 : 0)             /* Glyph table */
                               + 1                                  /* Number of DBCS LeadByte ranges */
                               + (dbcs_range_count ? 256 : 0)       /* offsets of lead byte sub tables */
                               + dbcs_table.size()                  /* LeadByte sub tables */
                               + 1;                                 /* Unknown flag */

    output.write(reinterpret_cast<char*>(&wc_table_offset), sizeof(wc_table_offset));

    output.write(reinterpret_cast<char*>(mb_table), sizeof(mb_table));

    uint16_t glyph_table_size = has_glyphs ? 256 : 0;
    output.write(reinterpret_cast<char*>(&glyph_table_size), sizeof(glyph_table_size));
    if (has_glyphs)
        output.write(reinterpret_cast<char*>(glyph_table), sizeof(glyph_table));

    output.write(reinterpret_cast<char*>(&dbcs_range_count), sizeof(dbcs_range_count));
    if (dbcs_range_count)
    {
        output.write(reinterpret_cast<char*>(lb_offsets), sizeof(lb_offsets));
    }
    if (dbcs_table.size())
    {
        output.write(reinterpret_cast<char*>(dbcs_table.data()), dbcs_table.size() * sizeof(uint16_t));
    }

    uint16_t unknown_flag = FileHeader.MaximumCharacterSize == 1 ? 0 : 4;
    output.write(reinterpret_cast<char*>(&unknown_flag), sizeof(unknown_flag));

    output.write(reinterpret_cast<char*>(wc_table), 65536 * FileHeader.MaximumCharacterSize);

    output.close();
    delete[] wc_table;

    return 0;
}
