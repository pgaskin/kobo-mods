# kobo-dotfile-hack
Since FW 4.17.13651, Kobo will import from dotfiles/folders. This is a LD_PRELOAD hack to fix that.

## Advantages
- You'll be able to use dotfolders for mods and files without having Kobo import the stuff in it.
- Unlike a patch, this is persistent (and there is less chance of accidentally disabling it).
- More control then a patch.
- Relatively simple.

## Disadvantages
- Needs a cross-compiler.
- Will not work completely if Kobo uses `fdopendir`.
- Hardcoded paths for dotfiles to show.
- Needs SSH/telnet access to remove (or a factory reset).

## Building

You can build kobo-dotfile-hack with [NickelTC](https://github.com/pgaskin/NickelTC) inside Docker using:

```sh
docker run --volume="$PWD:$PWD" --user="$(id --user):$(id --group)" --workdir="$PWD" --env=HOME --entrypoint=make --rm -it docker.io/geek1011/nickeltc:1.0
```

Or, on the host using:

```sh
make CROSS_COMPILE=/path/to/nickeltc/bin/arm-nickel-linux-gnueabihf-
```

## Testing
To test this on any Linux machine, run `make -f Makefile.native test`.
