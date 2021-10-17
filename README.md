# NMEUtil

A modding thing for MP2D `DEMO v0.1.21.4.1A 2021`.

# Why?

The game was taken down for 'legal reasons',

So... why not let the community do the improvements? :p

# How to use?

- Unpack all files from the [release zip](https://github.com/nkrapivin/NMEUtil/releases/latest) into the game folder.

- Edit `Initial.lua` to your liking, might also want to peek at other lua files.

- Run `NMEUtil.exe` and enjoy the game!

# How to revert?

Simply run the game as usual, via `PlayPrime2D.exe` instead of `NMEUtil.exe`.

The game will ignore any mods.

# Caveats?

- No live update, you have to restart the game for Initial.lua to be reloaded.

  ...or you can write your own live update if you want, it's not hard.

- The NMEv4 API is not documented *at all*.

  ...I am working on it, by dumping parts of the game's code, this will take some time.

  (not my fault that the engine is private!)

- NMA files (aka the game assets) cannot be edited.

  ...I won't work on this, don't ask.

# How to build?

Download Visual Studio 2022 or newer and build the entire solution.

Then copy `NMEPayload.dll`, `NMEUtil.exe`, `Initial.lua` and other Lua files into the game folder.

You can uncomment the `while (!IsDebuggerAttached...` line so the game would freeze while you attach a VS debugger.

(NOTE: You must attach a debugger to the `PlayPrime2D.exe` process, not to the injector `NMEUtil.exe`!)

# Third-Party Notices

- Lua 5.4.1 Copyright (C) 1994-2020 Lua.org, PUC-Rio,
  R. Ierusalimschy, L. H. de Figueiredo, W. Celes
  https://www.lua.org

- MinHook, by TsudaKageyu and contributors,
  https://github.com/TsudaKageyu/minhook

# Credits

- VULKAN SYSTEMS - help with x64dbg/Cheat Engine stuff

- mysterious funny question mark individual - an *amazing* icon for the injector
