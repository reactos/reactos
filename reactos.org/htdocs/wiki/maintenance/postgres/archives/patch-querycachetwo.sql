CREATE TABLE querycachetwo (
  qcc_type          TEXT     NOT NULL,
  qcc_value         SMALLINT NOT NULL  DEFAULT 0,
  qcc_namespace     INTEGER  NOT NULL  DEFAULT 0,
  qcc_title         TEXT     NOT NULL  DEFAULT '',
  qcc_namespacetwo  INTEGER  NOT NULL  DEFAULT 0,
  qcc_titletwo      TEXT     NOT NULL  DEFAULT ''
);
CREATE INDEX querycachetwo_type_value ON querycachetwo (qcc_type, qcc_value);
CREATE INDEX querycachetwo_title      ON querycachetwo (qcc_type,qcc_namespace,qcc_title);
CREATE INDEX querycachetwo_titletwo   ON querycachetwo (qcc_type,qcc_namespacetwo,qcc_titletwo);

