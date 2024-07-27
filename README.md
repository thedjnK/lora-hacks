# Unofficial RS1xx firmware

This is an unofficial firmware for the RS1xx sensor. It supports:

* Bluetooth 
* LoRa 
* On-board si7021 sensor 
* (Custom) external battery measurement 
* (Custom) external sensors (bme680, etc.) 
* (Custom) external IR LED for controlling one specific model of mitsubishi heavy industries HVAC 
* (Custom) external garage door (or any I/O) control
* LoRa MCUmgr protocol 
* Remotely (LoRa) or locally (Bluetooth) changing of device settings (LoRa keys, external battery measurement offset, device name) 
* (Optional) MCUboot bootloader and firmware update support (over Bluetooth, or LoRa if you're feeling *risky*)

Programming of this firmware involves the use of a hammer which will void your device's warranty.

No support or documentation is provided for this code.

This code is not released under a FOSS license and code contained in this repository must not be used or posted anywhere else.
