
CREATE SEQUENCE category_id_seq;

CREATE TABLE category (
  cat_id       INTEGER  NOT NULL  PRIMARY KEY DEFAULT nextval('category_id_seq'),
  cat_title    TEXT     NOT NULL,
  cat_pages    INTEGER  NOT NULL  DEFAULT 0,
  cat_subcats  INTEGER  NOT NULL  DEFAULT 0,
  cat_files    INTEGER  NOT NULL  DEFAULT 0,
  cat_hidden   SMALLINT NOT NULL  DEFAULT 0
);

CREATE UNIQUE INDEX category_title ON category(cat_title);
CREATE INDEX category_pages ON category(cat_pages);

