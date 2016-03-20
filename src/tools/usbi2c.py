#!/usr/bin/python
#
# Copyright (c) 2016, Igor Stoppa <igor.stoppa@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#

'''
Simple script for driving the USB-to-I2C adapter based on the FTDI FT232R.

References:
Adapter:      http://www.robot-electronics.co.uk/htm/usb_i2c_tech.htm
Chip:         http://www.ftdichip.com/Products/ICs/FT232R.htm
I2C protocol: http://www.robot-electronics.co.uk/i2c-tutorial
'''

def parse_args():
    '''
    Process command line options.
    '''
    import argparse

    def auto_int(x):
        return int(x, 0)

    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--address', type=auto_int,
                        help='The 7bit address of the slave.')
    parser.add_argument('-p', '--port', default="/dev/ttyUSB0",
                        help="Serial port provided by the adapter. "
                             "Must have sufficient rights to open it R/W.")
    subparsers = parser.add_subparsers(dest='subcommand')
    #  subparser for write
    parser_write = subparsers.add_parser('write')
    parser_write.add_argument('-r', '--register', type=auto_int,
                              required=True,
                              help='Hex id of the target register - 1byte.')
    parser_write.add_argument('data', nargs = '*', type=auto_int,
                              help='Device dependent amount of data.')
    #  subparser for read
    parser_read = subparsers.add_parser('read')
    parser_read.add_argument('-r', '--register', type=auto_int, required=True,
                             help='Hex id of the target register - 1byte.')
    parser_read.add_argument('data', type=auto_int,
                             help='Device dependent amount of data.')
    return parser.parse_args()


def get_serial(port):
    '''
    Open the port with the adapter parameters.
    '''
    try:
        import serial
        ser = serial.Serial(port=port,
                            baudrate=19200,
                            parity=serial.PARITY_NONE,
                            stopbits=serial.STOPBITS_TWO,
                            bytesize=serial.EIGHTBITS,
                            timeout=0.500,
                           )
    except OSError as e:
        import sys
        print  e
        sys.exit("Could not open " + port)
    return ser


def write(ser, addr, reg, data):
    '''
    Use the adapter-specific protocol for writing out data.
    '''
    I2C_AD1 = 0x55  # R/W one or more bytes for 7bits addresses.
    ser.write(chr(I2C_AD1) + chr(addr) + chr(reg) + chr(len(data)) +
              "".join([chr(i) for i in data]))
    # Read the retval. Leaving it in the buffer of the PIC will cause
    # read mismatches in following read operations.
    if ser.read() is 0:
        import sys
        sys.exit("Failure writing.")


def read(ser, addr, reg, data):
    '''
    Use the adapter-specific protocol for reading in data.
    '''
    I2C_AD1 = 0x55  # R/W one or more bytes for 7bits addresses.
    ser.write(chr(I2C_AD1) + chr(addr + 1) + chr(reg) + chr(data))
    return " ".join(['0x' + format(ord(i), '02x') for i in ser.read(data)])


if __name__ == "__main__":
    args = parse_args()
    ser = get_serial(port=args.port)
    if args.subcommand == 'write':
        write(ser=ser, addr=args.address, reg=args.register, data=args.data)
    if args.subcommand == 'read':
        print read(ser=ser, addr=args.address, reg=args.register,
                   data=args.data)
