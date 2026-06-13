Command Line Argument Parser
----------------------------

Parsing command line arguments to a console application is a common problem. 
This library handles the common task of reading arguments from a command line 
and filling in the values in a type.

To use this library, define a class whose fields represent the data that your 
application wants to receive from arguments on the command line. Then call 
Utilities.Utility.ParseCommandLineArguments() to fill the object with the data 
from the command line. Each field in the class defines a command line argument. 
The type of the field is used to validate the data read from the command line. 
The name of the field defines the name of the command line option.

The parser can handle fields of the following types:

- string
- int
- uint
- bool
- enum
- array of the above type

For example, suppose you want to read in the argument list for wc (word count). 
wc takes three optional boolean arguments: -l, -w, and -c and a list of files.

You could parse these arguments using the following code:

class WCArguments
{
    public bool lines;
    public bool words;
    public bool chars;
    public string[] files;
}

class WC
{
    static void Main(string[] args)
    {
        WCArguments parsedArgs = new WCArguments();
        if (!Utilities.Utility.ParseCommandLineArguments(args, parsedArgs)) 
        {
            // error encountered in arguments. Display usage message
            System.Console.Write(Utilities.Utility.CommandLineArgumentsUsage(typeof(WCArguments)));
        }
        else
        {
            // insert application code here
        }
    }
}

So you could call this aplication with the following command line to count 
lines in the foo and bar files:

    wc.exe /lines /files:foo /files:bar

The program will display the following usage message when bad command line 
arguments are used:

    wc.exe -x

Unrecognized command line argument '-x'
    /lines[+|-]                         short form /l
    /words[+|-]                         short form /w
    /chars[+|-]                         short form /c
    /files:<string>                     short form /f
    @<file>                             Read response file for more options

That was pretty easy. However, you realy want to omit the "/files:" for the 
list of files. The details of field parsing can be controled using custom 
attributes. The attributes which control parsing behaviour are:

CommandLineArgumentAttribute 
    - controls short name, long name, required, allow duplicates
DefaultCommandLineArgumentAttribute 
    - allows omition of the "/name".
    - This attribute is allowed on only one field in the argument class.

So for the wc.exe program we want this:

class WCArguments
{
    public bool lines;
    public bool words;
    public bool chars;
    [Utilities.Utility.DefaultCommandLineArgument]
    public string[] files;
}

class WC
{
    static void Main(string[] args)
    {
        WCArguments parsedArgs = new WCArguments();
        if (!Utilities.Utility.ParseCommandLineArguments(args, parsedArgs)) 
        {
            // error encountered in arguments. Display usage message
            System.Console.Write(Utilities.Utility.CommandLineArgumentsUsage(typeof(WCArguments)));
        }
        else
        {
            // insert application code here
        }
    }
}

So now we have the command line we want:

    wc.exe /lines foo bar

This will set lines to true and will set files to an array containing the 
strings "foo" and "bar".

The new usage message becomes:

    wc.exe -x

Unrecognized command line argument '-x'
    /lines[+|-]                         short form /l
    /words[+|-]                         short form /w
    /chars[+|-]                         short form /c
    @<file>                             Read response file for more options
    <files>

If you don't want to display the results to the Console you can also provide
a delegate which will be called when the parser reports errors during parsing.

Cheers,
Peter Hallam
C# Compiler Developer
Microsoft Corp.

