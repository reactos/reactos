@ALTER TABLE {PREFIX}suppress DROP INDEX url_idx;
CREATE INDEX url_idx on {PREFIX}suppress (host, ip);
