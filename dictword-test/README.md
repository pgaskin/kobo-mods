# dictword-test
This tool gets the prefix used for word lookup in the dictionary directly from libnickel. It works without any patches or conflicts by using a lot of LD_PRELOAD and dynamic linker hacks.

Japanese (Kanji) is not supported (it uses a different function, and is only called on the Japanese dictionaries anyways).

This was last tested on 4.19.14123, but it should continue to work for the forseeable future.

Theoretically, it should be possible to run this on a non-kobo ARM device (i.e. in QEMU), but this hasn't been tested. The biggest possible issue with doing this would be the reads to `/dev/ntx_io` and `/sys/class/power_supply/mc13892_bat/status`, but it should work fine without those (and it can be patched out if needed).

**To build:**

You can build dictword-test with [NickelTC](https://github.com/pgaskin/NickelTC) inside Docker using:

```sh
docker run --volume="$PWD:$PWD" --user="$(id --user):$(id --group)" --workdir="$PWD" --env=HOME --entrypoint=make --rm -it ghcr.io/pgaskin/nickeltc:1.0
```

Or, on the host using:

```sh
make CROSS_COMPILE=/path/to/nickeltc/bin/arm-nickel-linux-gnueabihf-
```

**To run (on a Kobo):**

```sh
chmod +x dictword-test.so && LD_PRELOAD=./dictword-test.so ./dictword-test.so
```

This can be run over SSH, telnet, or anything else.

**Usage:**

```
Usage: LD_PRELOAD=./dictword-test.so ./dictword-test.so word_utf8...
```

**Example output:**

```
[root@(none) pgaskin]# LD_PRELOAD=./dictword-test.so ./dictword-test.so "test"
Storing arguments
Initializing QCoreApplication
Loading libnickel
Loading DictionaryParser::htmlForWord
Calling DictionaryParser::htmlForWord
Intercepting getHtml
{"test", "te"},
```

```
[root@(none) pgaskin]# LD_PRELOAD=./dictword-test.so ./dictword-test.so "é"
Storing arguments
Initializing QCoreApplication
Loading libnickel
Loading DictionaryParser::htmlForWord
Calling DictionaryParser::htmlForWord
Intercepting getHtml
{"é", "éa"},
// {"", err(empty string not supported)},
```
