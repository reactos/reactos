-- Creates table math used for caching TeX blocks.  Needs to be run
-- on old installations when adding TeX support (2002-12-26)
-- Or, TeX can be disabled via $wgUseTeX=false in LocalSettings.php

-- Note: math table has changed, and this script needs to be run again
-- to create it. (2003-02-02)

DROP TABLE IF EXISTS /*$wgDBprefix*/math;
CREATE TABLE /*$wgDBprefix*/math (
  -- Binary MD5 hash of the latex fragment, used as an identifier key.
  math_inputhash varbinary(16) NOT NULL,
  
  -- Not sure what this is, exactly...
  math_outputhash varbinary(16) NOT NULL,
  
  -- texvc reports how well it thinks the HTML conversion worked;
  -- if it's a low level the PNG rendering may be preferred.
  math_html_conservativeness tinyint NOT NULL,
  
  -- HTML output from texvc, if any
  math_html text,
  
  -- MathML output from texvc, if any
  math_mathml text,
  
  UNIQUE KEY math_inputhash (math_inputhash)

) /*$wgDBTableOptions*/;
