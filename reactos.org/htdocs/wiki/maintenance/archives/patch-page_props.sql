-- Name/value pairs indexed by page_id
CREATE TABLE /*$wgDBprefix*/page_props (
  pp_page int NOT NULL,
  pp_propname varbinary(60) NOT NULL,
  pp_value blob NOT NULL,

  PRIMARY KEY (pp_page,pp_propname)
) /*$wgDBTableOptions*/;

