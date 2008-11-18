-- Patch for email notification on page changes T.Gries/M.Arndt 11.09.2004

-- A new column 'wl_notificationtimestamp' is added to the table 'watchlist'.
-- When a page watched by a user X is changed by someone else, an email is sent to the watching user X
-- if and only if the field 'wl_notificationtimestamp' is '0'. The time/date of sending the mail is then stored in that field.
-- Further pages changes do not trigger new notification mails as long as user X has not re-visited that page.
-- The field is reset to '0' when user X re-visits the page or when he or she resets all notification timestamps
-- ("notification flags") at once by clicking the new button on his/her watchlist page.
-- T. Gries/M. Arndt  11.09.2004 - December 2004

ALTER TABLE /*$wgDBprefix*/watchlist ADD (wl_notificationtimestamp varbinary(14));
