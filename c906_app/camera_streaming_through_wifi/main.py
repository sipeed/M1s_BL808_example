import socket
import numpy as np
import cv2 as cv


if __name__ == '__main__':
    # 创建tcp服务端套接字
    # 参数同客户端配置一致，这里不再重复
    tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # 设置端口号复用，让程序退出端口号立即释放，否则的话在30秒-2分钟之内这个端口是不会被释放的，这是TCP的为了保证传输可靠性的机制。
    tcp_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)

    # 给客户端绑定端口号，客户端需要知道服务器的端口号才能进行建立连接。IP地址不用设置，默认就为本机的IP地址。
    tcp_server.bind(("", 8888))

    # 设置监听
    # 128:最大等待建立连接的个数， 提示： 目前是单任务的服务端，同一时刻只能服务与一个客户端，后续使用多任务能够让服务端同时服务与多个客户端
    # 不需要让客户端进行等待建立连接
    # listen后的这个套接字只负责接收客户端连接请求，不能收发消息，收发消息使用返回的这个新套接字tcp_client来完成
    tcp_server.listen(128)

    # 等待客户端建立连接的请求, 只有客户端和服务端建立连接成功代码才会解阻塞，代码才能继续往下执行
    # 1. 专门和客户端通信的套接字： tcp_client
    # 2. 客户端的ip地址和端口号： tcp_client_address
    tcp_client, tcp_client_address = tcp_server.accept()

    # 代码执行到此说明连接建立成功
    print("客户端的ip地址和端口号:", tcp_client_address)

    while True:
        # 接收客户端发送的数据, 这次接收数据的最大字节数是4
        recv_data = tcp_client.recv(4)
        mjpeg_len = int.from_bytes(recv_data, 'little')
        print("recv len: ", mjpeg_len)
        tcp_client.send(recv_data)
        recv_data_mjpeg = b''
        remained_bytes = mjpeg_len
        while remained_bytes > 0:
            recv_data_mjpeg += tcp_client.recv(remained_bytes)
            remained_bytes = mjpeg_len - len(recv_data_mjpeg)
        print("recv stream success")
        if recv_data_mjpeg[:2] != b'\xff\xd8' \
                or recv_data_mjpeg[-2:] != b'\xff\xd9':
            continue
        mjpeg_data = np.frombuffer(recv_data_mjpeg, 'uint8')
        img = cv.imdecode(mjpeg_data, cv.IMREAD_COLOR)
        cv.imshow('stream', img)
        if cv.waitKey(1) == 'q':
            exit(0)

    # 关闭服务与客户端的套接字， 终止和客户端通信的服务
    tcp_client.close()

    # 关闭服务端的套接字, 终止和客户端提供建立连接请求的服务 但是正常来说服务器的套接字是不需要关闭的，因为服务器需要一直运行。
    # tcp_server.close()
