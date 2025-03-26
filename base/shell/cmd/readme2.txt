General Overview of How Things Work
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
First it comes into _main in cmd.c(1811).  The command line params are taking in and if it is unicode it uses CommandLineToArgvW.
This can cause a problem on older machines and that is why we have our own custom _CommandLineToArgvW to help this along.
We pull in the launch directory as the initial dir and set that in _tchdir.  We make a handle to the default console out using CreateFile.

Then we call Initialize().  Here we need to load ntdll.dll if it isn't loaded (windows 9x machines).
We also setup some global vars like default io handles and nErrorLevel and set %prompt% to $P$G.
This is where all command lines switches given to cmd on startup are done.

From here main calls ProcessInput().  This is where cmd loops for getting input and doing the commands.
First it checks to see if there is a batch file(note: there is a global struct "bc" which is NULL when not processing a batch file)
and if there is it will pull a new line from that file.  If not, then it will wait for input.
Currently there is some stuff for set /a in there, which might stay there or see if we can find a better spot.

Once there is input taken in from the command line it is sent into ParseCommandLine().
In here we fist check for aliases and convert if need be.
Then we look for redirections using GetRedirection() which will remove any redirection symbols.
and pass back info about where to redirect.
from this info it will do some switching around with the handles for where things go and send them as need be.
personally i dont like this code and i tried to change it before but failed.
it is confusing to me and i dont understand why a lot of it is there but apparently it is needed.

It sends the new string without any redirection info into DoCommand(). In this function we just look to see what should be done.
There is one of 2 things that could happen.
1) we fnd the matching command and send it off to that commands little section.
2) we dont find it so we send it to Execute() and see if it is a file that we can do something.

Execute will try to launch the file using createprocess and falls back on shellexecute.
It calls a function called SearchForExecutable() to find the full path name and looks in all the correct locations like PATH,
 current folder, windows folder.  If it cant find it, just fails and prints out a message.

Some useful functions that are used a lot:

split() - splits a string into an array of string on spaces that aren't inside quotes. which you need to call freep() on later t clean up.
//Split it´s used to take the Arguments from Command Line, it´s the best option for almost all the cases.
//If the Command has special needs as Dir, it´s better to make a Parser INSIDE that Command(as DIR has)
//Dont get mad(as i did): Split() can be find in Misc.c file.Really easy to follow.
//Also remember split() receives the Command Line, but the Command Line WITHOUT command name.

splitspace()-split a string into an array of string using spaces as splitters.which you need to call freep() on later to clean up.
//This is the son of split() for commands that manage in the same way "/" and "\" when are INSIDE the paths.
//i.e move works in the same way with: move C:\this/is\a/mess C:\i/know,and with move C:/this/is/a/mess C:/i/know
//Other commands DOESNT.
//You can find also in misc.c

IsValidPathName(), IsExistingFile(), IsExistingDirectory() - all do what you would expect.
PagePrompt() -  ask them to hit a key to continue
FilePromptYN[A]() - ask them a yes or no question
