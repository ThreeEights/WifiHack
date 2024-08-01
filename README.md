# WifiHack
Test code for Raspberry Pi Pico W WiFi chip

# The Issue
Something somewhere behaves differently between the Raspberry Pi Pico
talking to an external CYW4343W chip and trying to access the on-board
CYW43439 chip on the Pico W.

When I request the chip ID from the external Infineon WiFi module, all
is well:  I get the results I expect.  But when I try to get the chip
ID from the on-board CYW43439 on the Pico W, I get garbage.

The Pico SDK includes code to work successfully with the on-board
CYW43439 on the Pico W.  The picowi code (described below) works with
both the Pico W and a plain Pico with the external CYW4343W.  I was
even able to hack the Zephyr spi_bitbang driver to work in 3-wire
mode, and that works.  But when I try to use the Zephyr PIO SPI driver
for the Pico and Pico W, it works correctly on the Pico but not on
the Pico W (he whined).

# Context
The goal is to support the Zephyr Infineon WiFi driver on the Pico W:
    zephyr/drivers/wifi/infineon and
    modules/hal/infineon/wifi-host-driver
The challenge is that the way to Pico W is built, the CYW43439 can only
be accessed using 3 SPI lines, CS, CLK, and a bidirectional Data line
on hard-wired GPIO lines.  So the only way to get that result is to use
the PIO feature of the RP2040 SOC.  To support this, I wrote a PIO-based
SPI driver for the Pico, already part of Zephyr:
    zephyr/drivers/spi/spi_rpi_pico_pio.c

An issue working with the Pico W is there's no practical way to access
the traces on the board to connect, for example, a logic analyzer for
monitoring the SPI bus.  Jeremy Bentham built a stand-alone WiFi driver
that works for both the Pico and Pico W:
    https://iosoft.blog/2022/12/06/picowi/
He got around the lack of access to the bus by wiring an external
module (based on the CYW4343W) to a vanilla Pico.  Since I thought
that was a pretty good idea, I did the same thing to work on the PIO-
based SPI driver.  I wired a MuRata 1DX module as described for my
testing.

# Building
This test program requires my development fork of Zephyr, which has
changes required to use an SPI bus with the Infineon driver.  The
source URL is:
    https://github.com/ThreeEights/zephyr/tree/airoc-wifi-spi
which should take you right to the airoc-wifi-spi branch in my
repository.

With that branch of Zephyr checked out, this demo application can be
built in the usual way.  For the Pico with the external module, the
build command is:
```
west build -p auto -b rpi_pico -- -DCMAKE_BUILD_TYPE=Debug
```
and for the Pico W:
```
west build -p auto -b rpi_pico/rp2040/w -- -DCMAKE_BUILD_TYPE=Debug
```

# Test Setups

## Raspberry Pi Pico with Off-Board WiFi
As in Jeremy Bentham's Web page, wire the external module (noting that
the "WI-FI" and "BLUETOOTH" labels on the module connectors
***ARE BACKWARDS!!***), selecting suitable Pico GPIOs for CS, CLK,
DATA (SIO), WIFI_ON, and HOST_WAKE.  As you can see in the
boards/rpi_pico.overlay file, I used:

| Use | GPIO |
| --- | --- |
| CS | 13 |
| CLK | 14 |
| SIO | 15 |
| REG_ON | 16 |
| HOST_WAKE | 15 |

(Note that HOST_WAKE, which the WiFi chip uses to notify the CPU with
and interrupt, needs to be the same as the data/SIO line.)  The clock
and data/SIO lines must also be assigned to the PIO in the pinctrl
section.

## Raspberry Pi Pico W
Nothing special should be required for the Pico W;  the pin assignments
are hard-wired and shown in the boards/rpi_pico_rp2040_w.overlay file.

# Test Results

One curiosity of the Infineon WiFi chips is that they start up in a
peculiar byte ordering mode:  out of a 4-byte (32-bit) command or
response, the data order is byte 2, 3, 0, then 1.  The TEST_READ
command is hard-coded in this byte order (the actual data is
`0x4000A004`).  The expected value of `0xFEEDBEAD` should be returned
as `0xBEADFEED`.  The first attempt to read that register normally
results in an invalid pattern;  subsequent attempts should return
the expected result.

## Raspberry Pi Pico with Off-Board WiFi
Using the vanilla Raspberry Pi Pico with the external muRata WiFi
module, the correct data is returned on the second request:

```
checkWiFi() called
  Set data line low - pin 15
WIFI_NODE=WIFI_NODE
On pin=16
Sleep 150 msec
*** Booting Zephyr OS build v3.7.0-275-gccb616c96733 ***
Turn on WiFi chip
Add a delay of 50 msec
Request chip ID
Chip ID=0x06 0x06 0x06 0x06 
Add a delay of 50 msec
Request chip ID
Chip ID=0xBE 0xAD 0xFE 0xED 
Add a delay of 50 msec
Request chip ID
Chip ID=0xBE 0xAD 0xFE 0xED 
Add a delay of 50 msec
Request chip ID
Chip ID=0xBE 0xAD 0xFE 0xED 
Turn WiFi off
```

## Raspberry Pi Pico W
However, on the Pico W, I get garbage:

```
checkWiFi() called
  Set data line low - pin 24
WIFI_NODE=WIFI_NODE
On pin=23
Sleep 150 msec
*** Booting Zephyr OS build v3.7.0-275-gccb616c96733 ***
Turn on WiFi chip
Add a delay of 50 msec
Request chip ID
Chip ID=0xFF 0xFF 0xD7 0xFF 
Add a delay of 50 msec
Request chip ID
Chip ID=0xFF 0xFF 0x5F 0xFF 
Add a delay of 50 msec
Request chip ID
Chip ID=0xFF 0xFF 0x5F 0xFF 
Add a delay of 50 msec
Request chip ID
Chip ID=0xFF 0xFF 0x5F 0xFF 
Turn WiFi off
```

(Though the garbage from the first request is different from the
second and subsequent requests.)
