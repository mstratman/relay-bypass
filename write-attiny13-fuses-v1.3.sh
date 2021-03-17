# NOTE: This disables the reset pin which makes regular programming
#       impossible. You'll need special equipment
# NOTE: This is for 1.2MHz (9.6 / 8)
avrdude -p t13 -P usb -c usbtiny -U lfuse:w:0x6A:m	-U hfuse:w:0xFE:m	-U lock:w:0xFF:m
