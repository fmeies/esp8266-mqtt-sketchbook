# Working on this sketchbook

Setup, library versions and what each sketch does are in `README.md`. Do not
duplicate any of it here — it drifts.

## Traps that are not visible from the code

**`sketch_callback` must be defined before `#include <AllSketches.h>`.** The
library calls it, so the definition has to come first. `SERIAL_PRINT` does not
exist yet at that point — use `Serial.println` inside the callback. Getting
this wrong produces "not declared in this scope" and is how `stranger_things`
went years without compiling.

**Third-party libraries are not vendored.** Adding one means pinning its
version in two places: the table in `README.md` and the `lib install` step in
`.github/workflows/ci.yml`. Miss the second and CI breaks; miss the first and
the repo documents a version nobody builds against.

## Verifying changes

```bash
./tests/test_user_config.sh   # host compiler, fast
./tests/compile_all.sh        # real ESP8266 toolchain, the one that counts
```

Always run the second one before claiming a change works. The host compiler is
far newer than the core's GCC 4.8.2 and accepts constructs the target rejects —
the `__has_include` guard in `user_config.h` passed every host test and could
never have built on hardware.
