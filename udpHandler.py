import queue
import socket
import time

import numpy as np
from utils import Util
import threading
from enum import IntEnum
from definitions import *


class UDPHandler:
    def __init__(self, remote_ip_addr, remote_port, chunk_size=25, timeout_sec=0.25, cmd_callback=None):
        self.ip = remote_ip_addr
        self.port = remote_port
        self.addr = (self.ip, self.port)
        self.chunk_size = chunk_size
        self.timeout = timeout_sec
        self.callback = cmd_callback

        self.data_queue = queue.Queue()
        self.thread = threading.Thread(target=self.__send_handler__)
        self.mutex = threading.Lock()

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.tcp_sock.bind(("", self.port))

        self.is_running = False
        self.data_request_pending = False

    def __del__(self):
        self.close()

    def add_to_queue(self, data: np.ndarray):
        data = Util.split_chunks_no_pad(data, self.chunk_size)
        for d in data:
            try:
                self.data_queue.put(d.flatten(), block=False, timeout=None)
            except queue.Full:
                print("Queue is full")

    def __send_from_queue(self, block=False, timeout=1) -> bool:
        try:
            data = self.data_queue.get(block=block, timeout=timeout)
        except queue.Empty:
            return False

        success = self.send(data=data)
        if success:
            self.data_queue.task_done()
        return success

    def __send_handler__(self):
        while self.is_running:
            data_tcp = None
            try:
                data_tcp = self.tcp_sock.recv(1)     # Listen for REQUEST_DATA flag
            except socket.timeout:
                pass
            except ConnectionResetError as e:
                print(e)
                self.callback(Command.RESTART)

            if data_tcp is not None:
                data_tcp = int.from_bytes(data_tcp, "little")
                if data_tcp == Command.REQUEST_DATA:  # Means send more data
                    with self.mutex:
                        self.data_request_pending = True
                elif data_tcp == Command.READY:
                    if self.callback:
                        self.callback(data_tcp)

            with self.mutex:
                if self.data_request_pending:
                    success = self.__send_from_queue(block=False)
                    self.data_request_pending = not success
            time.sleep(0.005)    # Sleep for 5 ms to stabilize and avoid loosing packets

    def send(self, command: Command or None = None, data: np.ndarray or None = None) -> bool:
        # For DEFAULTS, command and data needs to be sent as a single packet
        if command == Command.DEFAULTS:
            d = np.zeros(len(data) + 1, dtype=np.uint16)
            d[0] = int(command)
            d[1:] = data
            print(command, d)
            data = d.tobytes()
            size = self.sock.sendto(data, self.addr)
            return size == len(data)

        if data is not None:
            data = data.tobytes()
            size = self.sock.sendto(data, self.addr)
            return size == len(data)

        if command is not None:
            print(command)
            cmd = int(command).to_bytes(length=1, byteorder="little")
            size = self.sock.sendto(cmd, self.addr)
            return size == 1

        return False

    def request(self, command: Command):
        self.sock.sendto(command, self.addr)
        data, addr = self.sock.recvfrom(command.value)
        return data

    def start(self):
        # Wait here until connection is established
        print("Waiting for TCP Client Device...")
        self.tcp_sock.listen(1)
        self.tcp_sock, addr = self.tcp_sock.accept()
        self.tcp_sock.settimeout(self.timeout)
        print(f"TCP Connection address: {addr}")

        self.is_running = True
        self.thread.start()

    def join(self):
        self.is_running = False
        if self.thread.is_alive():
            self.thread.join()

    def close(self):
        self.sock.close()
        self.join()
