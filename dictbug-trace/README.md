# dictbug-trace
DON'T USE THIS IF YOU DON'T NEED IT!

**Build:**

You can build dictbug-trace with [NickelTC](https://github.com/pgaskin/NickelTC) inside Docker using:

```sh
docker run --volume="$PWD:$PWD" --user="$(id --user):$(id --group)" --workdir="$PWD" --env=HOME --entrypoint=make --rm -it docker.io/geek1011/nickeltc:1.0
```

Or, on the host using:

```sh
make CROSS_COMPILE=/path/to/nickeltc/bin/arm-nickel-linux-gnueabihf-
```

**Run:**

```sh
mv libdictbug-trace.so /usr/local/pgaskin/
echo " /usr/local/pgaskin/libdictbug-trace.so" >> /etc/ld.so.preload
reboot
logread -f
```

**Sample dictionary:**
You can also use the sample dictionary (compile it with dictutil) to test a few things with images.

```sh
dictutil pack dicthtml
dictutil install -ldt dicthtml.zip
```
