# dictword-test
This tool gets the prefix used for word lookup in the dictionary directly from libnickel. It works without any patches or conflicts by using a lot of LD_PRELOAD and dynamic linker hacks.

Japanese (Kanji) mode is supported by passing the `-j` flag.

This was last tested on 4.45.23697, but it should hopefully continue to work for the forseeable future.
There were major changes since 4.19.14123, so if you are running an older version, please use an earlier commit.

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
chmod +x dictword-test.so LD_PRELOAD=./dictword-test.so ./dictword-test.so
```

This can be run over SSH, telnet, or anything else.

**Usage:**

```
Usage: LD_PRELOAD=./dictword-test.so [-j] ./dictword-test.so word_utf8...
Use -j to enable Japanese mode
```

**Example output:**

```

# Non-Japanese mode (default)
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

# Japanese mode (-j flag)
[root@(none) pgaskin]# LD_PRELOAD=./dictword-test.so ./dictword-test.so "日本語" "これ" "コレ" "あった" "人々" "あゝ"
Storing arguments
Initializing QCoreApplication
Loading libnickel
Loading DictionaryParser::htmlForWord (_ZNK16DictionaryParser11htmlForWordERK7QStringb)
Calling DictionaryParser::htmlForWord
Intercepting getHtml
{"日本語", "日"},
Calling DictionaryParser::htmlForWord
Intercepting getHtml
{"これ", "これ"},
Calling DictionaryParser::htmlForWord
Intercepting getHtml
{"コレ", "コレ"},
Calling DictionaryParser::htmlForWord
Intercepting getHtml
{"あった", "あっ"},
Calling DictionaryParser::htmlForWord
Intercepting getHtml
{"人々", "人々"},
Calling DictionaryParser::htmlForWord
Intercepting getHtml
{"あゝ", "あゝ"},
```

