This script adds a pdb file to the specified symbol store.

 Usage:

 %~0 [-u] [Path]


 -u [Path]   Download a remote file (zip or rpm) and push pdbs.
             NOTE: Path MUST BE a URL. 
 [Path]      Push pdbs, Path can be pdb, zip or rpm or a folder.
             NOTE: If one pdb is chosen, make sure .inf file is
                   in the same directory.

USED ENVIRONMENT VARIABLES:

 %%DSTORE_LOCATION%%  - The symbol store path, that pdb files
                        will be pushed to.

 %%DSTORE_BUILD_TAG%% - Will serve as a tag for future versions,
                        pushed with the pdb as a message.(Optional)
                        if not configured, the default is "eng".

NOTE: 1. 7zip should be installed and the path to 7z.exe should be
         configured in the push.cmd file.
      2. Path to symstore.exe, comes with (WDK), should be configured
         in the push.cmd file.

Edit the pushpdb.cmd with any text editor and configure the values.
