# MOD Duo Controller

This is the source code for the MOD Duo controller (sometimes referred to as "HMI").

If you don't know what this refers to, see [wiki.mod.audio/wiki/Device_Settings](https://wiki.mod.audio/wiki/Device_Settings).

## Building

There are no external dependencies besides `arm-none-eabi-` GCC and [mod-controller-proto](https://github.com/mod-audio/mod-controller-proto) (used as git submodule).

So building can be done with:

```
# adjust if not using debian-based distribution
sudo apt install gcc-arm-none-eabi

# clone recursively (note the --recursive, important!)
git clone --recursive https://github.com/mod-audio/mod-duo-controller.git

# change dir to where code is
cd mod-duo-controller

# build firmware
make modduo
```

The generated firmware file will be placed inside the `out/` subdirectory.

## Deploying

You can deploy HMI firmware with the `hmi-update` command included inside the MOD OS.
Just copy the firmware binary into the MOD unit over SSH and run this script with the binary filename as argument.

For example:

```
# Copy firmware file to MOD unit
scp -O out/mod-controller.bin root@192.168.51.1:/tmp/

# Update HMI firmware
ssh root@192.168.51.1 "hmi-update /tmp/mod-controller.bin"

# Restart mod-ui service
ssh root@192.168.51.1 "systemctl stop mod-ui"
```

## License

MOD Duo Controller is licensed under AGPLv3+, see [LICENSE](LICENSE) for more details.
