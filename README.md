# mmmojo

## Overview
mmmojo is a library for interprocess communication (IPC) based on chromium mojo.

## Build
Build mmmojo is the same as build Chromium

For example on Windows:

After checking out chromium, branch should need to be greater than m105, 
Because mmmojo uses raw_ptr in source(e.g. branch tags/108.0.5359.125):

```
cd chromium\src\third_party
```

Clone this repository into third_party or place mmmojo sources in `//third_party/mmmojo`

Back to src directories:

```
cd ...
```

Make sure you are now in the chromium\src directory:

```
git apply third_party/mmmojo/chromium_build_gn.patch
```

If you want to support some systems like windows 7, you can run this to improve stability:

```
git apply third_party/mmmojo/platform_thread_win.patch
```

Build debug version after m108(because `DCHECK_CURRENTLY_ON` is used in chromium many places, but the adaptation pollution is too much):
```
git apply third_party/mmmojo/browser_thread.patch
```

Then run `gn args` to create and edit the build configuration:

```
gn args out/Debug --filters=//third_party/mmmojo
```

For a typical debug build the contents may be as simple as:

```
is_debug = true
```

Now targets can be built:

```
ninja -C out/Debug third_party/mmmojo

# if you want to build flmojo
ninja -C out/Debug third_party/mmmojo:flmojo
```

