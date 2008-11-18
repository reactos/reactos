CREATE FUNCTION page_deleted() RETURNS TRIGGER LANGUAGE plpgsql AS
$mw$
BEGIN
DELETE FROM recentchanges WHERE rc_namespace = OLD.page_namespace AND rc_title = OLD.page_title;
RETURN NULL;
END;
$mw$;

CREATE TRIGGER page_deleted AFTER DELETE ON page
  FOR EACH ROW EXECUTE PROCEDURE page_deleted();

