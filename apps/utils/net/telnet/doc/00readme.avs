Hi!

Now I come back to telnet source, and make some changes, wich you suggest
to me:

1.  telnet.rc renamed to telnet.cfg
2.  I change syntax of 'keys' command (but I did'nt found a tool for edit
    msg*.bin files - so it's remain unchanged). Syntax are

        keys load keymapname [file]
        keys display
        keys switch number


I fix some 'political' ;) problem with charmap, now we (citizens of xUSSR)
have koi8, koi8r and koi8u(RFC on draft) on UNIX, wich are diff's on 6 or
8 letters; cp866 and many (3 or 4) very near to cp866 on DOS.

So, I rewrite code to able a charmap addition like a keymap done.

And I make more smart command line processing, look at telCommandLine().

And last: my english is not so good :( to rewrite documentation, but there are
things, wich would be described - look on next page. I think that you will
translate my english to more understable, ok?

and now is a list of files, wich I touch


                            old                         new

ANSIPRSR.CPP        32763  05.10.97  11:09    33237  24.12.97  17:42
ANSIPRSR.H           3311  04.09.97   0:25     3410  23.12.97  13:18
KEYTRANS.CPP         9504  28.05.97  22:43    26547  03.02.98  21:33
KEYTRANS.H           8020  25.01.97  16:06     8090  03.02.98  19:53
TNCLASS.CPP         13663  17.08.97  23:55    13891  03.02.98  20:09
TNCLASS.H            1112  01.06.97  14:19     1233  03.02.98  20:09
TNMAIN.CPP          12668  02.10.97  20:38    16610  03.02.98  21:22
TNNET.CPP            3445  01.06.97  14:21     3474  23.12.97  13:16
TNPARSER.CPP        17653  05.10.97  11:09    17715  23.12.97  18:03
TNPARSER.H           2129  01.06.97  14:22     2188  23.12.97  13:25

KEYS.CFG            erased
TELNET.CFG          new

TELNET.IDE          65810  26.10.97  16:53    66118  03.02.98  21:34


I was start my work with file telc2b4s.zip with size 132619 bytes, and now send
to you just files, wich I touch.

with best regards
Andrei V. Smilianets

smile@head.aval.kiev.ua
22:25 03 Feb 1998



There are all of my changes (from 2.04b), wich have to be described:

1. command line (telnet>) processing

   a 'keys' command

        was
                keys keymapname [file]
        new
                keys load keymapname [file] // mean unchanged
                keys display                // display a list of loaded keymaps
                keys switch number          // switch to keymap

   more smart command processing

        command might be writed shortly

          cl[ose]
          op[en]
          ke[ys]
          qu[it]

        subcommands of 'keys'

          l[oad]
          d[isplay]
          s[witch]

        synonym of '?' -> h[elp]

2. file 'keys.cfg' renamed to 'telnet.cfg'

3. Added codepage conversion, look [charmap]

4. completely changed conception of telnet.cfg

   Now you can define multiple keymaps, character maps, combine it in your
   ways.

   file is splitted into following sections:

    [COMMENT]
     ...
    [END COMMENT]

        it is for comment a big part of text. can be nested.
        in text also work:

           ; - first printable character in line, which is completelly
               ignored.
           // - like C++ comment

    [GLOBAL]
    ...
    [END GLOBAL]

        mean of [global] unchanged

    [KEYMAP name]
    ...
    [END KEYMAP]
        'name' - is a keymap name for reference. in 'name' you can use
        any char exept spaces, '+', ':' and ']'. '+' and ':' reserved for
        CONFIG section.
        body is a sequence of key definition:

           <vk_name> [keymodifier[+keymodifier[+...]]] <keytranslation>

        example:
           VK_F1 RIGHT_ALT+RIGHT_CTRL this_would_print

        vk_name is an ASCII string equivalent to an entry in [GLOBAL].

        valid keymodifiers are:
          RIGHT_ALT
          LEFT_ALT
          RIGHT_CTRL
          LEFT_CTRL
          SHIFT
          ENHANCED

        Undefined enhanced keys will use the non-enhanced definition.

        keytranslation is the string you want printed for the key.
        The notation ^[ can be used to denote an escape character.
        Any ASCII value can be represented by

            \nnn where nnn is a 3 digit decimal ASCII value or
            \xhh where hh is a 2 digit hexadecimal ASCII value.

        Leading zeros may not be omitted.
        A value of \000(\x00) will not be transmitted.

        note: In order to have both left and right alt have the same
        action, you must create a separate def for left and right.


    [CHARMAP name]
    ...
    [END CHARMAP]
        'name' - is a charmap name for reference. requirements is the same
        as for keymap name.
        body is a sequence of char conversion definition:

           <host_char> <console_char>

        where host_char is a char received from host, and console_char
        is a char, which would be displayed on console.

        The main purpose of it is a conversion between differents code
        pages, for example, on former USSR part of world most unix's hosts
        uses 'koi8' code page, and on W95 machines - 866 code page and
        (as say I.Ioannou)  Greece has the same problem with 737 and 928
        code-pages.


        Any ASCII value can be represented by

            \nnn where nnn is a 3 digit decimal ASCII value or
            \xhh where hh is a 2 digit hexadecimal ASCII value.

        Leading zeros may be omitted.
        A value of \000(\x00) will not be accepted.

        look for example at [charmap koi8-cp866].

    [CONFIG name]
    ...
    [END CONFIG]
        'name' - is a configuration name for reference. requirements is
        the same as for keymap name.

        you must define one with name 'default', which will be used as
        default.

        in body of this part you can combine keymaps and set charmap,
        format is:

           KEYMAP name_list [: <vk_name> [keymodifier[+keymodifier[+...]]] ]

             where
                  name_list:
                     keymap_name
                     keymap_name '+' name_list

                  keymap_name is a name of [KEYMAP]

             You can specify multiple keymaps, for first (mean default)
             you can not define ': <vk_name> ...' part, but for rests
             (secondary) you must!
             The ': <vk_name> ...' part define a key for switch to this
             keymap.

             Assigning a switching key to first (default) keymap will be
             ignored, but you can switch to by pressing second time switch
             key of current keymap.

             If a key not found in switched keymap, a program will be look
             for it in default keymap. So, you can redefine only needed keys
             in secondary keymaps.

           CHARMAP name_list

                where
                      name_list:
                                charmap_name
                                charmap_name '+' name_list

                      charmap_name is a name of [CHARMAP]


             define wich charmap(s) is to use.

           examples:
              [config default]
                 keymap default
              [end config]

              [config linux]
                 keymap default + linux
              [end config]

              [config default_koi8]
                 keymap default
                 keymap koi8u + koi8r : VK_/ RIGHT_ALT // russian keyboard
                 keymap koi8u         : VK_. RIGHT_ALT // ukranian

                 charmap koi8-cp866
              [end config]

              [config linux_koi8]
                 keymap default + linux
                 keymap koi8u + koi8r : VK_/ RIGHT_ALT // russian keyboard
                 keymap koi8u         : VK_. RIGHT_ALT // ukranian

                 charmap koi8-cp866 + koi8u-cp866
              [end config]

            so, for switch to russian keyboard just press RIGHT_ALT and '/'.
            and, for switch back to default press it again.


