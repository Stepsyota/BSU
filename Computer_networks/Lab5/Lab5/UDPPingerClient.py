from time import perf_counter
from socket import *

serverName = "127.0.0.1"
serverPort = 12000

# создаём UDP-сокет
clientSocket = socket(AF_INET, SOCK_DGRAM)

# устанавливаем тайм-аут 1 секунда
clientSocket.settimeout(1)

print("UDP Pinger Client Started\n")

for i in range(1, 11):
    # сообщение с номером пакета + время отправки
    message = f"Ping {i} {perf_counter()}"

    try:
        # время отправки
        start = perf_counter()

        # отправляем пакет
        clientSocket.sendto(message.encode(), (serverName, serverPort))

        # ждём ответ
        modifiedMessage, serverAddress = clientSocket.recvfrom(1024)

        # время получения
        end = perf_counter()
        rtt = end - start

        print(f"Reply from {serverAddress}: {modifiedMessage.decode()}  RTT = {rtt:.6f} sec")

    except timeout:
        print("Request timed out")

clientSocket.close()