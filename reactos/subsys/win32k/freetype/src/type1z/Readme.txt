This directory contains an experimental Type 1 driver that will ultimately
replace the "official" one in "src/type1".

This driver doesn't provide a mini Postscript interpreter, but uses
pattern matching in order to load data from fonts. It works better and
faster than the official driver, but will replace it only when we finish
the auto-hinting module..

You don't need to compile it to support Type 1 fonts, the driver should
co-exist peacefully with the rest of the engine however..
