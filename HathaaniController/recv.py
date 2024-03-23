import socket
import numpy as np

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(('', 8888))

d, addr = s.recvfrom(13)
print(f"received: {d} from {addr}")