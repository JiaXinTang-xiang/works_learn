#!/usr/bin/env python3
"""Send a fixed UART pattern without camera or vision code."""

import time

import serial


PORT = "/dev/ttyTHS1"
BAUDRATE = 921600

# Exactly 54 bytes. The first two bytes match the application frame header.
TEST_FRAME = bytes([
    0xA5, 0x34, 0x02, 0x01, 0x00, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xF0, 0x0F,
    0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
    0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x5A, 0xA5, 0x12, 0x34, 0x56, 0x78, 0x00, 0xFF,
])


def main():
    print(f"Opening {PORT} at {BAUDRATE}, 8N1")
    print(f"TX {len(TEST_FRAME)} bytes: {TEST_FRAME.hex(' ')}")

    with serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0,
        write_timeout=1,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    ) as uart:
        uart.reset_output_buffer()
        while True:
            written = 0
            for value in TEST_FRAME:
                count = uart.write(bytes((value,)))
                uart.flush()
                written += count
            print(f"safe byte-write={written}/{len(TEST_FRAME)}")
            time.sleep(1)


if __name__ == "__main__":
    main()
