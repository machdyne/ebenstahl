# Ebenstahl Storage Device

Ebenstahl is a desktop storage controller with sockets for up to 16 [MMOD](https://github.com/machdyne/mmod) modules. When paired with FRAM, EEPROM or SuperFlash MMODs, it can be used to build a low-cost long-term storage array.

![Ebenstahl](https://github.com/machdyne/ebenstahl/blob/c97c9766c0a3bc06164430417d3006f85a538926/ebenstahl.png)

This repo contains schematics, USB-MSC firmware and a 3D-printable case.

Find more information on the [Ebenstahl product page](https://machdyne.com/product/ebenstahl-storage-device/).

Also see the [FERRIT](https://github.com/machdyne/ferrit) project for more storage.

**Note: The EEPROM and Flash drivers are experimental and wear-leveling is not supported.**

### Supported Boards

 * [Ebenstahl](https://machdyne.com/product/ebenstahl-storage-device/)
 * [Graustahl](https://machdyne.com/product/graustahl-storage-device/)
 * [Kaltstahl](https://machdyne.com/product/kaltstahl-storage-device/)
 * [Blaustahl](https://machdyne.com/product/blaustahl-storage-device/)

To use this firmware with Graustahl, Kaltstahl or Blaustahl you will need to set the appropriate board definition in ebenstahl.h. For Blaustahl will you also need to comment out FRAM\_BIG in drv\_fram.c.

In all cases you will need to configure the LUN mappings in mapper.c.

You can build the firmware from source, if you have [pico-sdk](https://github.com/raspberrypi/pico-sdk) installed:

```
$ cd firmware/ebenstahl
$ mkdir build
$ cd build
$ cmake .. && make
```

## License

The contents of this repo are released under the [Lone Dynamics Open License](LICENSE.md).
