CREATE TABLE page_restrictions (
  pr_page      INTEGER       NULL  REFERENCES page (page_id) ON DELETE CASCADE,
  pr_type   TEXT         NOT NULL,
  pr_level  TEXT         NOT NULL,
  pr_cascade SMALLINT    NOT NULL,
  pr_user   INTEGER          NULL,
  pr_expiry TIMESTAMPTZ      NULL
);
ALTER TABLE page_restrictions ADD CONSTRAINT page_restrictions_pk PRIMARY KEY (pr_page,pr_type);

