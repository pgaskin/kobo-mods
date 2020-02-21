# dictbug-trace
DON'T USE THIS IF YOU DON'T NEED IT!

**Build:**

```sh
make NILUJE=/toolchain/arm-nickel-linux-gnueabihf DOCKER=podman
```

If you are using docker, set DOCKER=docker. If you are building directly on the host, leave it out entirely and set NILUJE accordingly. If you are using a custom toolchain/sysroot (which isn't one of NiLuJe's), set CROSS_PREFIX instead and set the Qt flags appropriately if not using pkg-config.

**Run:**

```sh
mv libdictbug-trace.so /usr/local/geek1011/
echo " /usr/local/geek1011/libdictbug-trace.so" >> /etc/ld.so.preload
reboot
logread -f
```
