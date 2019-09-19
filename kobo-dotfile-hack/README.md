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
To build, `make CROSS_PREFIX=/path/to/kobo/toolchain/bin/arm-whatever-linux-gnueabihf-`. For convenience, you can build using @NiLuJe's toolchain inside Docker with `docker run --rm -it geek1011/kobo-toolchain:latest` then `apt install make; git clone https://github.com/geek1011/kobo-mods; cd kobo-mods/kobo-dotfile-hack; make CROSS_PREFIX=arm-nickel-linux-gnueabihf-`.

## Testing
To test this on any Linux machine, run `make -f Makefile.native test`.
