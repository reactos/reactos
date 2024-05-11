/***
*stdargv.c - standard & wildcard _setargv routine
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       processes program command line, with or without wildcard expansion
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <limits.h>
#include <mbstring.h>
#include <stdlib.h>



// In the function below, we need to ensure that we've initialized the mbc table
// before we start performing character transformations.
static void do_locale_initialization(char)    throw() { __acrt_initialize_multibyte(); }
static void do_locale_initialization(wchar_t) throw() { /* no-op */                    }

static char*    get_command_line(char)    throw() { return _acmdln; }
static wchar_t* get_command_line(wchar_t) throw() { return _wcmdln; }

static char**&    get_argv(char)    throw() { return __argv;  }
static wchar_t**& get_argv(wchar_t) throw() { return __wargv; }

static errno_t expand_argv_wildcards(
    _In_z_               char**  const argv,
    _Out_ _Deref_post_z_ char*** const expanded_argv) throw()
{
    return __acrt_expand_narrow_argv_wildcards(argv, expanded_argv);
}

static errno_t expand_argv_wildcards(
    _In_z_               wchar_t**  const argv,
    _Out_ _Deref_post_z_ wchar_t*** const expanded_argv) throw()
{
    return __acrt_expand_wide_argv_wildcards(argv, expanded_argv);
}



/***
*static void parse_cmdline(cmdstart, argv, args, argument_count, character_count)
*
*Purpose:
*       Parses the command line and sets up the argv[] array.
*       On entry, cmdstart should point to the command line,
*       argv should point to memory for the argv array, args
*       points to memory to place the text of the arguments.
*       If these are nullptr, then no storing (only counting)
*       is done.  On exit, *argument_count has the number of
*       arguments (plus one for a final nullptr argument),
*       and *character_count has the number of bytes used in the buffer
*       pointed to by args.
*
*Entry:
*       Character *cmdstart - pointer to command line of the form
*           <progname><nul><args><nul>
*       Character **argv - where to build argv array; nullptr means don't
*                       build array
*       Character *args - where to place argument text; nullptr means don't
*                       store text
*
*Exit:
*       no return value
*       int *argument_count - returns number of argv entries created
*       int *character_count - number of characters used in args buffer
*
*Exceptions:
*
*******************************************************************************/


// should_copy_another_character helper functions
// should_copy_another_character is *ONLY* checking for DBCS lead bytes to see if there
// might be a following trail byte.  This works because the callers are only concerned
// about escaped quote sequences and other codepages aren't using those quotes.
static bool __cdecl should_copy_another_character(char const c) throw()
{
    // This is OK for UTF-8 as a quote is never a trail byte.
    return _ismbblead(c) != 0;
}

static bool __cdecl should_copy_another_character(wchar_t) throw()
{
    // This is OK for UTF-16 as a quote is never part of a surrogate pair.
    return false;
}

template <typename Character>
static void __cdecl parse_command_line(
    Character*  cmdstart,
    Character** argv,
    Character*  args,
    size_t*     argument_count,
    size_t*     character_count
    ) throw()
{
    *character_count = 0;
    *argument_count  = 1; // We'll have at least the program name

    Character c;
    int copy_character;                   /* 1 = copy char to *args */
    unsigned numslash;              /* num of backslashes seen */

    /* first scan the program name, copy it, and count the bytes */
    Character* p = cmdstart;
    if (argv)
        *argv++ = args;

    // A quoted program name is handled here. The handling is much
    // simpler than for other arguments. Basically, whatever lies
    // between the leading double-quote and next one, or a terminal null
    // character is simply accepted. Fancier handling is not required
    // because the program name must be a legal NTFS/HPFS file name.
    // Note that the double-quote characters are not copied, nor do they
    // contribute to character_count.
    bool in_quotes = false;
    do
    {
        if (*p == '"')
        {
            in_quotes = !in_quotes;
            c = *p++;
            continue;
        }

        ++*character_count;
        if (args)
            *args++ = *p;

        c = *p++;

        if (should_copy_another_character(c))
        {
            ++*character_count;
            if (args)
                *args++ = *p; // Copy 2nd byte too
            ++p; // skip over trail byte
        }
    }
    while (c != '\0' && (in_quotes || (c != ' ' && c != '\t')));

    if (c == '\0')
    {
        p--;
    }
    else
    {
        if (args)
            *(args - 1) = '\0';
    }

    in_quotes = false;

    // Loop on each argument
    for (;;)
    {
        if (*p)
        {
            while (*p == ' ' || *p == '\t')
                ++p;
        }

        if (*p == '\0')
            break; // End of arguments

        // Scan an argument:
        if (argv)
            *argv++ = args;

        ++*argument_count;

        // Loop through scanning one argument:
        for (;;)
        {
            copy_character = 1;

            // Rules:
            // 2N     backslashes   + " ==> N backslashes and begin/end quote
            // 2N + 1 backslashes   + " ==> N backslashes + literal "
            // N      backslashes       ==> N backslashes
            numslash = 0;

            while (*p == '\\')
            {
                // Count number of backslashes for use below
                ++p;
                ++numslash;
            }

            if (*p == '"')
            {
                // if 2N backslashes before, start/end quote, otherwise
                // copy literally:
                if (numslash % 2 == 0)
                {
                    if (in_quotes && p[1] == '"')
                    {
                        p++; // Double quote inside quoted string
                    }
                    else
                    {
                        // Skip first quote char and copy second:
                        copy_character = 0; // Don't copy quote
                        in_quotes = !in_quotes;
                    }
                }

                numslash /= 2;
            }

            // Copy slashes:
            while (numslash--)
            {
                if (args)
                    *args++ = '\\';
                ++*character_count;
            }

            // If at end of arg, break loop:
            if (*p == '\0' || (!in_quotes && (*p == ' ' || *p == '\t')))
                break;

            // Copy character into argument:
            if (copy_character)
            {
                if (args)
                    *args++ = *p;

                if (should_copy_another_character(*p))
                {
                    ++p;
                    ++*character_count;

                    if (args)
                        *args++ = *p;
                }

                ++*character_count;
            }

            ++p;
        }

        // Null-terminate the argument:
        if (args)
            *args++ = '\0'; // Terminate the string

        ++*character_count;
    }

    // We put one last argument in -- a null pointer:
    if (argv)
        *argv++ = nullptr;

    ++*argument_count;
}



extern "C" unsigned char* __cdecl __acrt_allocate_buffer_for_argv(
    size_t const argument_count,
    size_t const character_count,
    size_t const character_size
    )
{
    if (argument_count >= SIZE_MAX / sizeof(void*))
        return nullptr;

    if (character_count >= SIZE_MAX / character_size)
        return nullptr;

    size_t const argument_array_size  = argument_count  * sizeof(void*);
    size_t const character_array_size = character_count * character_size;

    if (SIZE_MAX - argument_array_size <= character_array_size)
        return nullptr;

    size_t const total_size = argument_array_size + character_array_size;
    __crt_unique_heap_ptr<unsigned char> buffer(_calloc_crt_t(unsigned char, total_size));
    if (!buffer)
        return nullptr;

    return buffer.detach();
}



/***
*_setargv, __setargv - set up "argc" and "argv" for C programs
*
*Purpose:
*       Read the command line and create the argv array for C
*       programs.
*
*Entry:
*       Arguments are retrieved from the program command line,
*       pointed to by _acmdln.
*
*Exit:
*       Returns 0 if successful, -1 if memory allocation failed.
*       "argv" points to a null-terminated list of pointers to ASCIZ
*       strings, each of which is an argument from the command line.
*       "argc" is the number of arguments.  The strings are copied from
*       the environment segment into space allocated on the heap/stack.
*       The list of pointers is also located on the heap or stack.
*       _pgmptr points to the program name.
*
*Exceptions:
*       Terminates with out of memory error if no memory to allocate.
*
*******************************************************************************/
template <typename Character>
static errno_t __cdecl common_configure_argv(_crt_argv_mode const mode) throw()
{
    typedef __crt_char_traits<Character> traits;

    if (mode == _crt_argv_no_arguments)
    {
        return 0;
    }

    _VALIDATE_RETURN_ERRCODE(
        mode == _crt_argv_expanded_arguments ||
        mode == _crt_argv_unexpanded_arguments, EINVAL);

    do_locale_initialization(Character());


    static Character program_name[MAX_PATH + 1];
    traits::get_module_file_name(nullptr, program_name, MAX_PATH);
    traits::set_program_name(&program_name[0]);

    // If there's no command line at all, then use the program name as the
    // command line to parse, so that argv[0] is initialized with the program
    // name.  (This won't happen when the program is run by cmd.exe, but it
    // could happen if the program is spawned via some other means.)
    Character* const raw_command_line = get_command_line(Character());
    Character* const command_line = raw_command_line == nullptr || raw_command_line[0] == '\0'
        ? program_name
        : raw_command_line;

    size_t argument_count  = 0;
    size_t character_count = 0;
    parse_command_line(
        command_line,
        static_cast<Character**>(nullptr),
        static_cast<Character*>(nullptr),
        &argument_count,
        &character_count);

    __crt_unique_heap_ptr<unsigned char> buffer(__acrt_allocate_buffer_for_argv(
        argument_count,
        character_count,
        sizeof(Character)));

    _VALIDATE_RETURN_ERRCODE_NOEXC(buffer, ENOMEM);

    Character** const first_argument = reinterpret_cast<Character**>(buffer.get());
    Character*  const first_string   = reinterpret_cast<Character*>(buffer.get() + argument_count * sizeof(Character*));

    parse_command_line(command_line, first_argument, first_string, &argument_count, &character_count);

    // If we are not expanding wildcards, then we are done...
    if (mode == _crt_argv_unexpanded_arguments)
    {
        __argc = static_cast<int>(argument_count - 1);
        get_argv(Character()) = reinterpret_cast<Character**>(buffer.detach());
        return 0;
    }

    // ... otherwise, we try to do the wildcard expansion:
    __crt_unique_heap_ptr<Character*> expanded_argv;
    errno_t const argv_expansion_status = expand_argv_wildcards(first_argument, expanded_argv.get_address_of());
    if (argv_expansion_status != 0)
        return argv_expansion_status;

    __argc = [&]()
    {
        size_t n = 0;
        for (auto it = expanded_argv.get(); *it; ++it, ++n) { }
        return static_cast<int>(n);
    }();

    get_argv(Character()) = expanded_argv.detach();
    return 0;
}



extern "C" errno_t __cdecl _configure_narrow_argv(_crt_argv_mode const mode)
{
    return common_configure_argv<char>(mode);
}

extern "C" errno_t __cdecl _configure_wide_argv(_crt_argv_mode const mode)
{
    return common_configure_argv<wchar_t>(mode);
}
