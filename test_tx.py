import socket
import struct
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

values = [1500, 1500, 1000, 1500, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
msg = struct.pack("!hhhhhhhhhhhhhhhh", *values)

while True:
    sock.sendto(msg, ("127.0.0.1", 7534))
    time.sleep(0.004)
