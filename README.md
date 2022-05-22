# kobo-mods
My Kobo mods which aren't [patches](https://github.com/pgaskin/kobopatch-patches/releases/latest).

Binaries for the latest commit can be downloaded [here](https://ci.appveyor.com/project/pgaskin/kobo-mods/build/artifacts).

| Mod | Description | Usage | Status |
| --- | --- | --- | --- |
| **[kobo-dotfile-hack-ng](./kobo-dotfile-hack-ng)** | Prevents Nickel from discovering and importing dotfiles/folders. Improved version of [kobo-dotfile-hack](./kobo-dotfile-hack). | General | Complete |
| **[NickelMenu](https://github.com/pgaskin/NickelMenu)** | Adds custom menu items to nickel. | General | Complete, Released |
| **[NickelSeries](./NickelSeries)** | Adds series (and subtitle) metadata support to libnickel. | General | Stable, Released |
| **[dictword-test](./dictword-test)** | This tool gets the prefix used for word lookup in the dictionary directly from libnickel. | Debugging | Complete |
| [kobo-dotfile-hack](./kobo-dotfile-hack) | Since FW 4.17.13651, Kobo will import from dotfiles/folders. This is a LD_PRELOAD hack to fix that. | General | Complete, Released, Superseded |
| [dictbug-trace](./dictbug-trace) | This LD_PRELOAD library logs some dictionary-related things to syslog. | Debugging | As-is |

Mods in **bold** are actively maintained.

**Status:**
- **POC:** Barely-working, work may or may not be continued, not stable.
- **As-is:** For testing only, updated as needed, may not be stable.
- **Experimental:** Basic features complete, may not be stable yet, major missing feature.
- **Beta:** All major features completed.
- **Stable:** All major features completed, stable.
- **Complete:** All planned features completed, stable.
- **Released:** Ready for general usage.

**Abandoned mods:**
- [kobopatch-livepatch](https://github.com/pgaskin/kobo-mods/commit/777cfe6637d69fdc59905c5c357f520f5e73ab28)
