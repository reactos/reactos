DROP INDEX entry_idx;
CREATE INDEX entry_idx ON {PREFIX}_entries(title);
