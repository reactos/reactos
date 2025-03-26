====================
ReactOS Applications
====================

This directory contains extra applications for ReactOS.
Make sure you also have a copy of the rest of the ReactOS
source before you attempt to build anything in this module.
It is to be placed under "modules" subdirectory of a trunk checkout.
The module requires to be enabled during the "configure" process.

To include the module in your build folder, run the configure script with the flags -DENABLE_ROSAPPS=1

# For Windows users

    configure.cmd -DENABLE_ROSAPPS=1

# For UNIX users

    ./configure.sh -DENABLE_ROSAPPS=1
