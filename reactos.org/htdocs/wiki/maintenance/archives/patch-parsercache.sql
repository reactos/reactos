--
-- parsercache table, for cacheing complete parsed articles 
-- before they are imbedded in the skin.
--

CREATE TABLE /*$wgDBprefix*/parsercache (
  pc_pageid INT(11) NOT NULL,
  pc_title VARCHAR(255) NOT NULL,
  pc_prefhash CHAR(32) NOT NULL,
  pc_expire DATETIME NOT NULL,
  pc_data MEDIUMBLOB NOT NULL,
  PRIMARY KEY (pc_pageid, pc_prefhash),
  KEY(pc_title),
  KEY(pc_expire)
) /*$wgDBTableOptions*/;
