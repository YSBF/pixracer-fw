import serial
import serial_datagram
import argparse
import threading
import time

error_count = 0
data_count = 0
datagram_count = 0

def receiver(port):
    global error_count, data_count, datagram_count
    while True:
        try:
            data = serial_datagram.read(port)
            data_count += len(data)
            datagram_count += 1
        except (serial_datagram.CRCMismatchError, serial_datagram.FrameError):
            error_count += 1

def parse_commandline_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('port', help='Serial port.')
    parser.add_argument('-b', '--baud', help='Serial baudrate', default=57600, required=False)
    return parser.parse_args()

def main():
    global error_count, data_count, datagram_count

    args = parse_commandline_args()
    port = serial.Serial(args.port, args.baud)

    t = threading.Thread(target=receiver, args=(port, ))
    t.start()

    while True:
        time.sleep(1)
        print("throughput: {} kB/s {} packet/s {} errors/s".format(data_count/1000.0, datagram_count, error_count))
        error_count = 0
        data_count = 0
        datagram_count = 0

if __name__ == '__main__':
    main()
