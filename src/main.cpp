#include <mbed.h>
#include "EthernetInterface.h"
#include "TCPSocket.h"

const int maxConnections = 5;

int main() {
    EthernetInterface * net = new EthernetInterface();
    if (!net) {
        printf("no network interface\n");
    }

    nsapi_size_or_error_t result = net->connect();
    if (result != 0) {
        printf("Connect failed: %d\n", result);
    }

    TCPSocket server;
    SocketAddress ip;
    net->get_ip_address(&ip);
    const char * ipAddr = ip.get_ip_address();
    printf("ip address: %s\n", ipAddr);
    ip.set_port(80);

    server.open(net);
    server.bind(ip);
    server.listen(maxConnections);

    int requestCounter = 0;
    while(1) {
        printf("request: %d\n", ++requestCounter);
        nsapi_error_t error = 0;
        TCPSocket * clientSocket;
        clientSocket = server.accept(&error);
        if (error != 0) {
            printf("accept failed: %d\n", error);
        }

        char rxBuf[512] = { 0 };
        nsapi_size_or_error_t recvResult;
        recvResult = clientSocket->recv(rxBuf, sizeof(rxBuf));
        if (recvResult < 0) { // todo what about 0?
            printf("recv error: %d\n", recvResult);
        }
        // printf("receive: size: %d data: %s\n", recvResult, rxBuf);

        char txBuf[512] = { 0 };
        char response[] = "howdy!";
        // fixme line ending \r\n
        sprintf(txBuf, "HTTP/1.1 200 OK\nContent-Length: %d\nContent-Type: text\nConnection: Close\n\n%s\n", strlen(response), response);
        printf("header: %s\n", txBuf);
        clientSocket->send(txBuf, strlen(txBuf));

        clientSocket->close();
    }
}