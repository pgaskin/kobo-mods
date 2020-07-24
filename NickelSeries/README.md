# NickelSeries

Adds native support for parsing series (and subtitle) metadata from EPUB files.

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

**Troubleshooting:**

- **How do I uninstall NickelSeries?**<br>
  Create a file named `ns_uninstall` (without any extension) in the KOBOeReader partition and reboot.
- **How do I uninstall NickelSeries manually?**<br>
  Connect over Telnet/SSH and delete `/usr/local/Kobo/imageformats/libns.so`. A
  hard factory reset will also remove it.
- **Series metadata is incorrect or missing for imported books.**<br>
  Please report the issue and post a copy of the OPF document from within the EPUB.
- **I installed NickelSeries and nothing changed!**<br>
  NickelSeries doesn't parse metadata for existing books. To do this, you can use [seriesmeta](https://github.com/pgaskin/kepubify/releases) with the argument `--no-persist`.
- **The series metadata changes after a reboot or USB connection.**<br>
  If you use Calibre or seriesmeta, those will take precedence.
- **Will this conflict with Calibre or Kobo-UNCaGED?**<br>
  No. For the initial import, NickelSeries prioritizes the `calibre:series` meta tags. For changes afterwards, NickelSeries doesn't override them.
- **My Kobo crashes when I try and import a book.**<br>
  This issue is unlikely to occur, but if it does, you can uninstall NickelSeries the usual way (see above). If possible, it would also be helpful if you could report the error and post the syslog.

Note that even though this mod includes some files from NickelMenu, it won't conflict with it (even if it is a different version or if NickelMenu is installed via LD_PRELOAD) due to the symbol visibility and scope. Although, due to limitations in the way failsafe works, if one triggers the failsafe, both NickelMenu and NickelSeries may be removed and need to be re-installed.

Also see pgaskin/kobo-plugin-experiments#6.
