# kobo-dotfile-hack-ng
Since FW 4.17.13651, Kobo will import from dotfiles/folders. This mod fixes that by making dotfiles invisible to nickel.

**Features:**
- Hides the dotfile itself, but not things under it accessed directly (this prevents automatic scanning and importing without breaking anything else).
- Doesn't use `LD_PRELOAD` or `/etc/ld.so.preload`.
- Can be uninstalled by creating a file named `dfh_uninstall` in the KOBOeReader partition.
- Persistent between firmware upgrades and sign-outs.
- Doesn't conflict with other mods.
