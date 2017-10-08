====================
ReactOS Applications
====================

This directory contains extra applicatons for ReactOS.
Make sure you also have a copy of the rest of the ReactOS
source before you attempt to build anything in this module.
It is to be placed under "modules" subdirectory of a trunk checkout.
No additional changes to build files are necessary, it'll be picked up
automatically.

To include "rosapps" in your build folder:
1. Copy the rosapps folder into the reactos\modules folder, or
2. Link reactos/modules/rosapps to rosapps

# For Windows users

    cd %%_ROSSOURCEDIR%%\reactos\modules
    mklink /j rosapps %%_ROSSOURCEDIR%%\rosapps

# For UNIX users

    cd \$$_ROSSOURCEDIR/reactos/modules
    ln -s \$$_ROSSOURCEDIR/rosapps rosapps
