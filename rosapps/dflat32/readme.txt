
How do I build D-Flat/32?

At present D-Flat/32 does not exist as a separate dll. But you can build an
example progam.

To build the example program (actually this is the FreeDOS Editor) you only
have to run 'make dflat32' from the rosapps base directory. The example
program is 'dflat32\edit.exe'.


What must be changed in D-Flat/32?

Things that have to be fixed (incomplete list):
  - key handling (replace dos keycodes by win32 console keycodes)
  - message queue should become a double linked list (it's an array now)
  - establish consitent naming conventions ('Df'/'DF' prefix) to avoid
    collisions (e.g. CreateWindow() --> DfCreateWindow())
  - fix short dos filename buffers
  - add code to register external window classes
  - implement recognition of current console screen size
  - fix remaining bugs
  - update documentation

Does it run on ReactOS?
The debugging version doesn't run on ReactOS yet. I haven't tried a version
without debugging code but I doubt it will run. I know that three console
functions are not implemented in ReactOS yet. They will be implemented soon.

If you have a question, drop me a note.


Eric Kohl   ekohl@rz-online.de
