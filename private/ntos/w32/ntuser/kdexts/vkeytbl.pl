#
# Vkeytbl.pl
#
# Pick up the VK definitions and normalize them.
# input: stdin
# output: stdout
#
# Created: hiroyama Jan 1999
#

#
# Firstly read all VK definitions
# and store them.
#
while (<>) {
   if (/^#define/ && /VK_/ && !/ERICSSON/) {
      chop;
      ($dummy, $symbol, $def) = split ' ';

      # if the definition is final, normalize the caps
      # in 0xXX form.
      $def =~ tr/A-FX/a-fx/ if $def =~ /^0[Xx]/;

      # normalize 0x0ff form to 0xff
      $def =~ s/^0x0([0-9a-f][0-9a-f])/0x\1/;

      #
      push(@lines, ($def . ' ' . $symbol));

      # remember the definition
      $value{$symbol} = $def;

   }
}

#
# Dereference the indirect definitions, like
# #define VK_ABC VK_OTHER
#
# VK_OTHER should have been registered in %value
# in the loop above. Now resolve them.
#
foreach $thisline (@lines) {
   ($def, $symbol) = split ' ', $thisline;
   $def = $value{$def} unless $def =~ /^0x/;
   push(@finish, ($def . ', "' . $symbol . '",'));
}

#
# Let's sort it and print !
#
print join("\n", sort @finish);
print "\n";

