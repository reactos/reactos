CREATE TABLE redirect (
  rd_from       INTEGER  NOT NULL  REFERENCES page(page_id) ON DELETE CASCADE,
  rd_namespace  SMALLINT NOT NULL,
  rd_title      TEXT     NOT NULL
);
CREATE INDEX redirect_ns_title ON redirect (rd_namespace,rd_title,rd_from);

