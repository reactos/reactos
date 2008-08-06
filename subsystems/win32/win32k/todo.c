Initialization -- Needs a total review for NT compatibility
  -DriverEntry
    - We are loaded once per session by smss.
	  - What functions does smss use to load us? When is DriverEntry actually called?
      - What should we do here?
  -NtGdiInit
    - When are we called and what should we do?
  -NtUserInitialize
    - Find out parameters and what we should do here.

System calls:
  -NtGdi compatible with 2003. Missing parameters only for NtGdiGetSpoolMessage.
    - Add stubs in /ntgdi/ and /ntddraw/
  -NtUser: Functions need to be researched for parameters. 
           We miss quite a few and most of the functions which are compatible in name/parameter numbers probably have ros-specific parameters/behaviour.
		   In short, a lot of research and review is needed.

gdi32 and user32
  - Need to be fixed to compile (not to use ros-specific system calls).
  - gdi32 is the easiest. Much is done but #if 0-ed due to remaining problems.
  - user32 needs a big overview (*cough*, rewrite, *cough*) and fixup.


Interaction with display drivers
 - Videoprt needs cleanup (rewriteeeeeeeee) and review. I have begun some major cleanup, but not done yet (mostly code style and unfscking).