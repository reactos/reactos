@DROP INDEX url_idx;
CREATE INDEX url_idx on {PREFIX}suppress (host, ip);
