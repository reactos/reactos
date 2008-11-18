--
-- Key to revision.rev_id
-- This field is used to add support for a tree structure (The Adjacency List Model)
--
-- 2007-03-04
--

ALTER TABLE /*$wgDBprefix*/revision
  ADD rev_parent_id int unsigned default NULL;
