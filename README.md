# Ebenstahl Storage Device

Ebenstahl is a desktop storage controller with sockets for up to 16 [MMOD](https://github.com/machdyne/mmod) modules. When paired with FRAM or EEPROM MMODs, it can be used to build a long-term storage array.

This repo contains schematics, USB-MSC firmware and a 3D-printable case.

Find more information on the [Ebenstahl product page](https://machdyne.com/product/ebenstahl-storage-device/).

## Firmware

The firmware is under development and all features aren't yet supported.

You can build the firmware from source, if you have [pico-sdk](https://github.com/raspberrypi/pico-sdk) installed:

```
$ cd firmware/ebenstahl
$ mkdir build
$ cd build
$ cmake .. -DPICO_DEFAULT_BOOT_STAGE2_FILE=$PICO_SDK_PATH/src/rp2040/boot_stage2/boot2_generic_03h.S
$ make
```

## License

The contents of this repo are released under the [Lone Dynamics Open License](LICENSE.md).
