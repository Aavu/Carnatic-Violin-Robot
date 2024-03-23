import socket
import numpy as np
import time

IP_ADDR = "10.2.1.177"
# IP_ADDR = "127.0.0.1"
PORT = 8888

shape = (25, 7)
data = np.arange(shape[0]*shape[1], dtype=np.int16).reshape(shape)
# print(data)
data = data.tobytes()
# print(data)
# print(np.frombuffer(data, dtype=np.int16).reshape(shape))

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(('', 8888))
s.settimeout(1)

# s.sendto(data, (IP_ADDR, PORT))

while True:
    try:
        d, addr = s.recvfrom(2)
    except socket.timeout:
        continue
    print(d.decode('utf-8'))
    if d.decode('utf-8') == "1":
        for i in range(10):
            s.sendto(data, (IP_ADDR, PORT))

# for i in range(1000):
#     data = np.arange(i, i+7, dtype=np.int16).tobytes()
#     s.sendto(data, (IP_ADDR, PORT))
#     time.sleep(0.01)

# d, addr = s.recvfrom(13)
# print(f"received: {d} from {addr}")
