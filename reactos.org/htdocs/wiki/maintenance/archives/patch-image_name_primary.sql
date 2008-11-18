-- Make the image name index unique

ALTER TABLE /*$wgDBprefix*/image DROP INDEX img_name;

ALTER TABLE /*$wgDBprefix*/image
  ADD PRIMARY KEY img_name (img_name);
