# NickelSeries

Adds native support for parsing series (and subtitle) metadata from EPUB files.

**[`Download`](https://pgaskin.net/kepubify/ns/)** &nbsp; **[`Documentation`](https://pgaskin.net/kepubify/ns/)**

**Features:**

- Works on EPUB and KEPUB files.
- Compatible with the Calibre Kobo plugin.
- Compatible with Calibre or EPUB3 series metadata (Calibre takes precedence).
- Will not override Nickel if Kobo ends up implementing it.
- Compatible with firmware 3.19.5761+.
- Survives firmware updates.
- Does not modify any system files, single file only.
- Easy to uninstall (either delete it over telnet/ssh or create a file named `ns_uninstall` in the KOBOeReader partition).
- Metadata appears instantly.
- Will also add subtitle metadata (EPUB3 or a Calibre custom column named `#subtitle`).

Note that even though this mod includes some files from NickelMenu, it won't conflict with it (even if it is a different version or if NickelMenu is installed via LD_PRELOAD) due to the symbol visibility and scope. Although, due to limitations in the way failsafe works, if one triggers the failsafe, both NickelMenu and NickelSeries may be removed and need to be re-installed.

Also see pgaskin/kobo-plugin-experiments#6.
