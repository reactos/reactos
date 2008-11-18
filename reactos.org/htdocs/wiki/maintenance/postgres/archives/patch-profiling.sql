CREATE TABLE profiling (
  pf_count   INTEGER         NOT NULL DEFAULT 0,
  pf_time    NUMERIC(18,10)  NOT NULL DEFAULT 0,
  pf_name    TEXT            NOT NULL,
  pf_server  TEXT            NULL
);
CREATE UNIQUE INDEX pf_name_server ON profiling (pf_name, pf_server);
