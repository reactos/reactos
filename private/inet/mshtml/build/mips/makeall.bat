for %%i in (debug ship dbship debug.rt ship.rt dbship.rt profile) do (pushd %%i & call make ffresh & popd)
