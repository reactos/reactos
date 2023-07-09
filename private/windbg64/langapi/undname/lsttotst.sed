#
# Convert a list of decorated names to an input file for testundn.
#
s/\(.*\)/{ 0x0800, "\1", "" },/
