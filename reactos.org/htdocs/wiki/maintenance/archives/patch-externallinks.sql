--
-- Track links to external URLs
--
CREATE TABLE /*$wgDBprefix*/externallinks (
  el_from int(8) unsigned NOT NULL default '0',
  el_to blob NOT NULL,
  el_index blob NOT NULL,
  
  KEY (el_from, el_to(40)),
  KEY (el_to(60), el_from),
  KEY (el_index(60))
) /*$wgDBTableOptions*/;

