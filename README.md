# kobo-mods
My Kobo mods which aren't [patches](https://github.com/geek1011/kobopatch-patches/releases/latest).

Binaries for the latest commit can be downloaded [here](https://ci.appveyor.com/project/geek1011/kobo-mods/build/artifacts).

| Mod | Description |
| --- | --- |
| [kobo-dotfile-hack](./kobo-dotfile-hack) | Since FW 4.17.13651, Kobo will import from dotfiles/folders. This is a LD_PRELOAD hack to fix that. |
| [dictword-test](./dictword-test) | This tool gets the prefix used for word lookup in the dictionary directly from libnickel. |
| [dictbug-trace](./dictbug-trace) | This LD_PRELOAD library logs some dictionary-related things to syslog. |
| [kobopatch-livepatch](./kobopatch-livepatch) | Live-patching nickel with ptrace. |
