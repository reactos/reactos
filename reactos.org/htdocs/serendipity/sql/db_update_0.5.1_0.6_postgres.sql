CREATE INDEX date_idx ON {PREFIX}entries (timestamp);

CREATE INDEX mod_idx ON {PREFIX}entries (last_modified);
