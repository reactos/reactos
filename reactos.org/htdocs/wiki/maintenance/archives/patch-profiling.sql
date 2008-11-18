-- profiling table
-- This is optional

CREATE TABLE /*$wgDBprefix*/profiling (
  pf_count int NOT NULL default 0,
  pf_time float NOT NULL default 0,
  pf_memory float NOT NULL default 0,
  pf_name varchar(255) NOT NULL default '',
  pf_server varchar(30) NOT NULL default '',
  UNIQUE KEY pf_name_server (pf_name, pf_server)
) ENGINE=HEAP;
