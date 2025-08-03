# Ebenstahl Storage Device

Ebenstahl is a desktop storage controller with sockets for up to 16 [MMOD](https://github.com/machdyne/mmod) modules. When paired with FRAM or EEPROM MMODs, it can be used to build a long-term storage array.

![Ebenstahl](https://github.com/machdyne/ebenstahl/blob/c97c9766c0a3bc06164430417d3006f85a538926/ebenstahl.png)

This repo contains schematics, USB-MSC firmware and a 3D-printable case.

Find more information on the [Ebenstahl product page](https://machdyne.com/product/ebenstahl-storage-device/).

## Firmware

The firmware is under development and all features aren't yet supported.

### Supported Boards

 * [Ebenstahl](https://machdyne.com/product/ebenstahl-storage-device/)
 * [Graustahl](https://machdyne.com/product/graustahl-storage-device/)
 * [Kaltstahl](https://machdyne.com/product/kaltstahl-storage-device/)
 * [Blaustahl](https://machdyne.com/product/blaustahl-storage-device/)

To use this firmware with Graustahl, Kaltstahl or Blaustahl you will need to comment out the EBENSTAHL definition in ebenstahl.h. For Blaustahl will you also need to comment out FRAM\_BIG in drv\_fram.c and change the size in mapper.c.

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
