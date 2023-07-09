#
# wm.pl
#
# pick up the window massage definitions and normalize them.
#

while (<>) {
   chop;
   if (/^#define/) {
      @words = split ' ';
      $_ = $words[1];

      #
      # WM_, CB_, LB_, LBCB_, EM_
      #
      # but not ends with one of:
      # "FIRST", "LAST", "ERR", "ERRSPACE"
      #
      if ((/^WM_/ || /^CB_/ || /^LB_/ || /^LBCB_/ || /^EM_/) && !/FIRST$/ && !/LAST$/ &&
            !/ERR$/ && !/ERRSPACE$/) {
         if ($done{$_} == 0) {
            $done{$_} = 1;
            print "#ifdef ", $_, "\n";
            print "    WM_ITEM(", $_, "),\n";
            print "#endif\n";
         }
      }
   }
}
