ALTER TABLE /*$wgDBprefix*/user
  ADD COLUMN user_editcount int;

-- Don't initialize values immediately... or should we?
-- They will be lazy-evaluated, or batch-filled via maintenance/initEditCount.php
