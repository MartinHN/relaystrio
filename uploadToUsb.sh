port="/dev/tty.usbserial-1120"

esptool="/Users/tinmarbook/Library/Arduino15/packages/esp32/tools/esptool_py/4.2.1/esptool"

# $esptool --chip esp32 --port $port --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 "/Users/tinmarbook/Dev/momo/relaystrio/build/relaystrio.ino.bootloader.bin" 0x8000 "/Users/tinmarbook/Dev/momo/relaystrio/build/relaystrio.ino.partitions.bin" 0xe000 "/Users/tinmarbook/Library/Arduino15/packages/esp32/hardware/esp32/2.0.5/tools/partitions/boot_app0.bin" 0x10000 "/Users/tinmarbook/Dev/momo/relaystrio/build/relaystrio.ino.bin"

# this erase flash first
$esptool --chip esp32 --port $port --baud 921600 --before default_reset --after hard_reset write_flash -e -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 "/Users/tinmarbook/Dev/momo/relaystrio/build/relaystrio.ino.bootloader.bin" 0x8000 "/Users/tinmarbook/Dev/momo/relaystrio/build/relaystrio.ino.partitions.bin" 0xe000 "/Users/tinmarbook/Library/Arduino15/packages/esp32/hardware/esp32/2.0.5/tools/partitions/boot_app0.bin" 0x10000 "/Users/tinmarbook/Dev/momo/relaystrio/build/relaystrio.ino.bin"
