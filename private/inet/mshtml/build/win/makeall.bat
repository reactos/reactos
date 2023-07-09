for %%i in (debug profile ship debug.96p ship.96p profile.96p debug.v ship.v profile.v) do (pushd %%i & call make ffresh & popd)
