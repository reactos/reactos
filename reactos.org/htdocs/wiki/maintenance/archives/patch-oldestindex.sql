-- Add index for "Oldest articles" (Special:Ancientpages)
-- 2003-05-23 Erik Moeller <moeller@scireview.de>

ALTER TABLE /*$wgDBprefix*/cur
   ADD INDEX namespace_redirect_timestamp(cur_namespace,cur_is_redirect,cur_timestamp);
