#!/usr/local/bin/python3

import socket

def client_program():
    host = "192.168.178.28"  
    port = 6666  

    client_socket = socket.socket()  
    client_socket.connect((host, port))  

    i = 0
    while i < 3:
        # Sent 3 requests
        message = "req"

        client_socket.send(message.encode())  
        data = client_socket.recv(1024).decode()  

        print('RNG: ' + data)  
        i=i+1

    # Disconnect client
    message = "end"

    client_socket.send(message.encode())  
    data = client_socket.recv(1024).decode()  

    print('End RNG: ' + data)  

    client_socket.close()  


if __name__ == '__main__':
    client_program()
