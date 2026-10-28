# Script Hook V SDK (not included)

The SDK's own readme says it **must not be redistributed**, so it is not part of this repository.

1. Download the SDK from <http://www.dev-c.com/gtav/scripthookv/> ("Script Hook V SDK").
2. Put these into this folder so the layout is:

```
sdk/
  inc/    enums.h  main.h  nativeCaller.h  natives.h  types.h
  lib/    ScriptHookV.lib
```

`build.bat` checks for `sdk\inc\main.h` and stops with a message if it is missing.
`.gitignore` excludes `sdk/inc/` and `sdk/lib/` so they cannot be committed by accident.

Terms from the SDK readme: scripts built with it must be intended to work **offline**, and `ScriptHookV.dll`
must not be redistributed with your plugin (link to the official download instead).
