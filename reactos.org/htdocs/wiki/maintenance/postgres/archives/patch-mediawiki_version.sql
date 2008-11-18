CREATE TABLE mediawiki_version (
  type         TEXT         NOT NULL,
  mw_version   TEXT         NOT NULL,
  notes        TEXT             NULL,

  pg_version   TEXT             NULL,
  pg_dbname    TEXT             NULL,
  pg_user      TEXT             NULL,
  pg_port      TEXT             NULL,
  mw_schema    TEXT             NULL,
  ts2_schema   TEXT             NULL,
  ctype        TEXT             NULL,

  sql_version  TEXT             NULL,
  sql_date     TEXT             NULL,
  cdate        TIMESTAMPTZ  NOT NULL DEFAULT now()
);

