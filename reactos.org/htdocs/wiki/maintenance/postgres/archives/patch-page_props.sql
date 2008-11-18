
CREATE TABLE page_props (
  pp_page      INTEGER  NOT NULL  REFERENCES page (page_id) ON DELETE CASCADE,
  pp_propname  TEXT     NOT NULL,
  pp_value     TEXT     NOT NULL
);
ALTER TABLE page_props ADD CONSTRAINT page_props_pk PRIMARY KEY (pp_page,pp_propname);
CREATE INDEX page_props_propname ON page_props (pp_propname);

