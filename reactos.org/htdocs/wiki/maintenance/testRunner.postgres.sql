--
-- Optional tables for parserTests recording mode
-- With --record option, success data will be saved to these tables,
-- and comparisons of what's changed from the previous run will be
-- displayed at the end of each run.
--
-- This file is for the Postgres version of the tables
--

-- Note: "if exists" will not work on older versions of Postgres
DROP TABLE IF EXISTS testitem;
DROP TABLE IF EXISTS testrun;
DROP SEQUENCE IF EXISTS testrun_id_seq;

CREATE SEQUENCE testrun_id_seq;
CREATE TABLE testrun (
  tr_id           INTEGER PRIMARY KEY NOT NULL DEFAULT nextval('testrun_id_seq'),
  tr_date         TIMESTAMPTZ,
  tr_mw_version   TEXT,
  tr_php_version  TEXT,
  tr_db_version   TEXT,
  tr_uname        TEXT
);

CREATE TABLE testitem (
  ti_run      INTEGER   NOT NULL REFERENCES testrun(tr_id) ON DELETE CASCADE,
  ti_name     TEXT      NOT NULL,
  ti_success  SMALLINT  NOT NULL
);  
CREATE UNIQUE INDEX testitem_uniq ON testitem(ti_run, ti_name);
