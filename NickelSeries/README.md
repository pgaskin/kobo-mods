# NickelSeries

Adds native support for parsing series metadata from EPUB files.

**Features:**
- Works on EPUB and KEPUB files.
- Compatible with the Calibre Kobo plugin.
- Compatible with Calibre or EPUB3 series metadata (Calibre takes precedence).
- Will not override Nickel if Kobo ends up implementing it.
- Compatible with firmware 3.19.5761+.
- Survives firmware updates.
- Does not modify any system files, single file only.

This mod is currently experimental. Although it has safety features built-in, use it at your own risk (you may need to factory reset if something goes wrong).

Note that even though this mod includes some files from NickelMenu, it won't conflict with it (even if it is a different version or if NickelMenu is installed via LD_PRELOAD) due to the symbol visibility and scope. Although, due to limitations in the way failsafe works, if one triggers the failsafe, both NickelMenu and NickelSeries may be removed and need to be re-installed.

**TODO:**
- Test it more.
- Refactor hook.
- Cleanup.

pgaskin/kobo-plugin-experiments#6
