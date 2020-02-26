# kobopatch-livepatch
Uses ptrace to live-patch nickel. Currently a work-in-progress (so far, everything's implemented, but only simple stuff has been tested). More documentation will come later. Prebuilt binaries can be downloaded from AppVeyor.

**Example:**

On the Kobo:

```sh
# Connect over telnet or SSH
# Copy kplived to a folder on the Kobo
# Note: A prebuilt KoboRoot.tgz is coming soon.
# You probably want to force WiFi on in developer options.

/path/to/kplived
```

On a computer:

```sh
# Extract the yaml files from a kobopatch-patches zip.
# Get the Kobo binaries from the firmware update zips.
# Note: You can also use most nickel patches with this.

/path/to/kplive the.kobo.ip.address ./libnickel.so.1.0.0 ./libnickel.so.1.0.0.yaml
```

**Usage:**

```
Usage: kplive [options] kplived_hostname[:port=7181] binary patch_file

Version:
  kobopatch-livepatch dev (kobopatch v0.14.2-0.20200226025105-71fa1456a9d6)

Options:
  -h, --help                    Show this help text
  -b, --kplived-binary string   The full path to the binary being patched (default: /usr/local/Kobo/{basename-of-source-binary}
```

```
Usage: kplived [options]

Version:
  kobopatch-livepatch dev

Options:
  -a, --addr string     The address to listen on (default ":7181")
  -h, --help            Show this help text
  -n, --nickel string   The process to patch (default "/usr/local/Kobo/nickel")

This MUST only be called once per instance of nickel.
```
