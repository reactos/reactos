==================
= ROSDOCS PRIMER =
================== [by your beloved librarian KJK::Hyperion]

HISTORY
=======
20 Jan 2003: initial version

FOREWORD
========
    This document should be written in RosDocs, to demonstrate its power. But
since RosDocs is, let's be frank, vaporware, and since I'm still looking for an
XML editor that 1) doesn't suck and 2) isn't written in Java (but this is a bit
redundant), *and* since I was supposed to finish this document some two weeks
ago, hand-formatted plaintext wins, for the moment. It should be a matter of
half a hour with lex+yacc to write a converter

    Also note that "Win32" and "NT" are trademarked. In this document you'll
never find mentions of "Win32" or "NT", but "Ros32" and "Native" - and you're
encouraged to do the same, except when exclusively talking about Microsoft's
implementation (for example, to document a function's availability on various
Microsoft products). In abbreviations and identifiers, don't use "w32", "win32"
or "nt", except for Microsoft-specific code and documentation, or as an
abbreviation of the families of system calls (i.e. Nt*, NtUser* and W32k*) -
in all other cases, use "r32", "ros32" and "ntv", respectively. Avoid using
"Windows" in the same contexts as well - use "ReactOS". And remember that
Alphonse Capone wasn't jailed for extortion and mass murder, but for tax
evasion

CONCEPTS
========
    RosDocs, in its current incarnation, is a fairly complex documentation
system, but that can still be managed and authored with simple and promptly
available tools, such as text editors and file managers (yet allowing for more
sophisticated tools to be developed in the future)

    It's not vital to grasp all the concepts behind RosDocs to start authoring
documentation for the ReactOS project, since one of the tenets of RosDocs is the
strict separation between content, storage and presentation. Should any of the
three be found flawed, the other two wouldn't need to be throwed away as well

    Let's spend a few more words about content, storage and presentation:

CONTENT
-------
    There's two ways to add content to RosDocs:

 - Doxygen comments. If you're going to write reference pages (think Unix man
   pages), this is the format you should get accostumed to. It consists of
   special comments, containing markup that a tool called Doxygen can extract
   into a variety of formats. If you already know JavaDoc, or the QT comment-
   based documentation system, Doxygen supports those as well

   There's currently no strict guidelines, since RosDocs is still being planned,
   so you have a great deal of freedom. Don't abuse it, though. Avoid any
   structural markup, except paragraphs, lists, etc. And, at the moment, don't
   worry about storage as discussed in another section of this document

 - DocBook XML documents. Doxygen can be abused to write manuals and books, but
   a better long-term solution is learning the DocBook XML DTD. You can also use
   DocBook for reference pages, but Doxygen is preferred for that. Also note
   that, at the moment, reference pages are more important than guides

   No guidelines yet for DocBook, either. In general, don't write books yet.
   Limit yourself to articles. A good starting point is writing short tutorials
   for the Knowledge Base, since no earth-shaking changes are expected in that
   field

STORAGE
-------
    This is where things get tough. Remember, though, that this specification is
draft at best. Future directions include:

 - support for DocBook books
 - filling the text of hyperlinks with the title of the target topic
 - SELECT queries, resolved at compile-time, to build tables of links

    All things that, with the very strict decoupling of content and storage
outlined here, would be impossible, or involve run-time processing. For this
reason, limit yourself to reference pages and Knowledge Base articles. Guides,
overviews and examples can wait

    That said, here are the storage concepts of RosDocs:

DOMAINS
    If you are familiar with other schemes featuring domains, such as Internet
host names, or the Java third-party classes, this is nearly the same. Otherwise,
read on

    Since some package names are awfully common, and since third-party
contributions are encouraged, it's necessary to compartment package names on a
vendor, product and, for complex products, feature basis. Such namespaces are
encoded as follows:

[ [ <vendor-specific subdivision>. ]<scope>. ]<vendor>

    The scope can be a product or book name, as in the following examples:

 - ReactOS Platform SDK: psdk.reactos
 - GCC manual: gcc.fsf
 - Bruce Eckel's Thinking in C++: ticpp.eckel

    Further, optional subdivisions are possible:

 - GCC internals manual: guts.gcc.fsf

    Note, however, that, at the moment, only the following domains are accepted:

 - psdk.reactos (ReactOS Platform SDK)
 - ssdk.reactos (ReactOS Subsystem Development Kit)
 - ddk.reactos (ReactOS Driver Development Kit)
 - kb.reactos (ReactOS Knowledge Base)

PACKAGES
    Packages are collections of topics and indexes. They are the base unit of
storage. A package may additionally contain one or more of the following items:

 - secondary indexes
 - a table of contents
 - configuration metadata

    Packages are an interface that exposes topics and indexes, they don't 
dictate a specific implementation, neither in their "source" nor in their
"compiled" form. Possible implementations of the compiled form ("engines")
include:

 - database on a remote server
 - filesystem directory
 - compressed archive

    There's no well-defined standard for the source form yet, but it's expected
to be a derivative of DocBook XML. Third parties can obviously choose other
formats than the future standard for the source form, but official ReactOS
documentation will have to be written in the standard

    For your documentation, you're free to organize your topics in as many
packages as you like. For ReactOS books and manuals, the following packages are
defined:

 - psdk.reactos domain:

    - ros32. The Ros32 subsystem's structure and boot sequence; the RPC APIs to
      the Windows and Console servers; general considerations on the API

    - err. Ros32 error codes; messages, parameters and meanings

    - base. Basic Ros32 APIs. These include file, device and console I/O and
      control; registry, memory, handles, thread, process and service
      management; DLL loading; and basic error handling

    - ui. Basic Ros32 user interface APIs. These include windows, MDI windows,
      window classes, resources, hooks, DDE, keyboard and mouse input, and
      standard controls

    - gdi. Ros32 GDI and printer spooler APIs

    - rtl. Ros32 Runtime Library support. These include string formatting,
      large integer support and interlocked memory access

    - ipc. Ros32 APIs to synchronization objects, shared memory, named and
      anonymous pipes, and mailslots

    - sec. Ros32 interfaces to access control; standard access rights for Ros32
      object types; GINA API and implementation; Network Providers API and
      implementation; general security considerations and guidelines

    - dbg. Ros32 debugger API; Ros32 SEH support

    - psapi. Process Status Helper API

    - tlhlp. Tool Helper API

    - commdlg. Common Dialog Box Library

    - commctrl. Common Controls Library

 - ssdk.reactos domain:

    - ntv. ReactOS Native architecture; system structure and boot sequence; RPC
      API to the Base server; the Process Environment Block; the Thread
      Environment Block; the Kernel/User Shared Data

    - err. NTSTATUS error codes; messages, parameters and meanings

    - obj. Native objects and handles; overview of predefined object types;
      Object Manager basics; the system objects namespace

    - sec. The Native security model; explanation of token objects; SIDs, ACEs
      and ACLs; generic access rights; standard access rights for kernel object
      types

    - seh. Structured Exception Handling internals

    - lpc. The Local Procedure Call protocol

    - dbg. Debugging interfaces, both kernel and user mode; the debugger LPC
      protocol

    - ntzw. System calls (Nt* and Zw*), both kernel and user mode

    - rtl. Runtime library interfaces, both kernel and user mode; list of
      supported C runtime interfaces, both kernel and user mode

    - ldr. The PE Loader API (Ldr*), both kernel and user mode

    - csr. The Client-Server Runtime API (Csr*); server modules API and
      implementation

    - nls. National Language Support API (Nls*), both kernel and user mode

    - ntuser. Native User Interface (NtUser*) system calls, both kernel and user
      mode

    - w32k. Native GDI (W32k*) system calls, both kernel and user mode

    - peexe. Structure and semantics of the PE executable format

 - ddk.reactos domain:

    - err. Bugcheck codes; messages, parameters and meanings

    - ke. The Kernel; architecture and API

    - hal. The Hardware Abstraction Layer; architecture and API

    - cc. The Cache Manager subsystem; architecture and API

    - cm. The Configuration Manager subsystem; architecture and API;
      implemented object types

    - ex. The Executive Support subsystem; architecture and API;
      implemented object types

    - io. The I/O Manager subsystem; architecture and API; implemented object
      types

    - kd. Kernel debugging; protocols and API

    - ki. Predefined interrupt handlers

    - lpc. The Local Procedure Call subsystem; architecture and API;
      implemented object types

    - mm. The Virtual Memory Manager subsystem; architecture and API;
      implemented object types

    - ob. The Object Manager subsystem; architecture and API; implemented
      object types

    - ps. The Process Structure Manager subsystem; architecture and API;
      implemented object types

    - se. The Security Reference Monitor subsystem; architecture and API;
      implemented object types

TOPICS
    Topics are the base unit of documentation. They contain the actual content,
and are organized in physical units of storage called packages

    [placeholder]

INDEXES
    Indexes are an addressing mechanism to retrieve a topic from a package.
Addresses in an index are strings, called keys, associated to each topic in
their source form. There's essentially two kinds of indexes:

 - Identification indexes. They provide a many-to-one mapping between keys
   and topics. That is, a topic can be pointed at by one or more keys in the
   same identification index. Identification indexes are typically used for
   unambiguous identification of topics

   Currently, a number of predefined identification indexes are defined.
   There's a strong bias towards developer's documentation at the moment, but
   it will be solved by specializing generic indexes. Here are the generic
   identification indexes currently defined:

    - page. Self-contained documents containing detailed information about a
      specific topic. Examples include Knowledge Base articles, reference
      pages of user commands, library functions, etc. They usually contain
      links to related references or sections

    - section. Any topic with structural meaning, i.e. not self-contained.
      They usually contain links to child sections or references

   For developer's documentation, the following specialization of the section
   index are defined:

    - subsection. A subsection of strongly-related topics
 
    - overview. Pages containing general information about a subsection

    - example. Pages containing code samples about a subsection

    - reference. Pages with an index of detailed information about a
      subsection. The distinction between "overview", "example" and "reference"
      indexes is necessary to simplify authoring, since, this way, the root
      subnodes of a subsection can be given the same key, as in the following
      example:

 TOC                       | Index[key]
---------------------------+-----------------------------
 User Interface            | <none>
  + ReactOS User Interface | section[rosui]
    + Windowing            | section[windowing]
       + Windows           | subsection[windows]
          + Overview       | overview[windows]
          + Examples       | example[windows]
          + Reference      | reference[windows]

      For an analogous reason, there's a further specialization for child nodes
      of reference sections to create indexes of functions, structures, macros,
      etc. These specialized indexes are:

       - functions. Functions other than class methods

       - structures. Types declared with the C/C++ struct or union constructs,
         and that have no methods

       - enumerations. Types declared with the C/C++ enum construct

       - types. Integral, array or pointer types with a special meaning

       - constants. Groups of macros that comply with the following
         requirements:
          - they have no parameters
          - they can be fully evaluated at compile time
          - they are strongly related. For example, they are interchangeable
            values for the same parameter, or flags, or both
          - they are referenced from more than one reference page. If they are
            referenced just once, they must be documented in the page that
            references them

       - errors. Specialization of the constants index. Constants used to return
         status codes

       - macros. Macros, or groups of macros, that comply with at least one of
         the following requirements:
          - they have parameters
          - they are used for conditional compilation
          - they are used as declaration modifiers
          - they have side effects
         Function aliases for type-generic string handling (_t* for the C
         runtime and *W/*A for the Ros32 API) are excluded, and must be
         documented as functions (in all their possible variants for the C
         runtime, and in their type-generic form for the Ros32 API). Macros
         returning l-values (e.g. errno) should be listed in the objects
         index

       - interfaces. Abstract classes

       - classes. Classes with an implementation

       - objects. Statically allocated memory objects. The type of such
         objects should be documented separatedly

       - messages. Window messages

       - notifications. Window messages delivered through WM_NOTIFY

    - guide. Root node of a guide/how-to/tutorial

 - Search indexes. They provide a one-to-many mapping between keys and topics.
   That is, a key in a search index can point to one or more topics. Search
   indexes are typically used for keyword searches among many topics

    [placeholder]

TOCS
    [placeholder]

CATALOGS
    [placeholder]

PRESENTATION
------------

AUTHORING
=========
    [placeholder]

USING DOXYGEN
-------------
    [placeholder]

USING DOCBOOK
-------------
    [placeholder]

GUIDELINES
----------
    [placeholder]
