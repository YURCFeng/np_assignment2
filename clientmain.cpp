#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <calcLib.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <vector>

#include "protocol.h"
#define MAX_RETRY 2
#define DEBUG
char *server_ip = "13.53.76.30";
int server_port = 5001;

int main(int argc, char *argv[])
{
    int sock;
	struct addrinfo hints,* p,* servinfo;
	char delim[] = ":";
	char *Desthost = strtok(argv[1], delim); // dizhi
	char *Destport = strtok(NULL, delim);
	int port = atoi(Destport); // 5000
	server_ip = Desthost;
	server_port = port;
	printf("Host %s, and port %d.\n", server_ip, server_port);

	int rv;
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_UNSPEC 表示支持IPv4和IPv6
	hints.ai_socktype = SOCK_DGRAM;

	 if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0)
    { 
        perror("getaddrinfo fail");
        exit(1);
    }
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	/* Do magic */
	char buf[1000];
	// int sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	// if (sock < 0)
	// {
	// 	perror("Failed to create socket");
	// 	exit(1);
	// }

	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
	{
		perror("Failed to set socket timeout");
		exit(1);
	}

	calcMessage msg;
	msg.type = htons(22);
	msg.message = htonl(0);
	msg.protocol = htons(17);
	msg.major_version = htons(1);
	msg.minor_version = htons(0);

	// struct sockaddr_in serv_addr;
	// serv_addr.sin_family = AF_INET;
	// serv_addr.sin_port = htons(server_port);
	// inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);
	int retry_count = 0;
	int n = 0;
	calcProtocol res;
	// socklen_t serv_len = sizeof(serv_addr);
	while (retry_count <= MAX_RETRY)
	{
		// 发送消息
		printf("Sending message\n");
		if (sendto(sock, &msg, sizeof(msg), 0,  p->ai_addr, p->ai_addrlen) < 0)
		{
			perror("Failed to send message\n");
			exit(1);
		}
		// memcpy
		n = recvfrom(sock, &buf, sizeof(buf), 0,  p->ai_addr, &(p->ai_addrlen));
		if (n < 0)
		{
			printf("Timeout, retrying...\n");
			retry_count++;
			continue;
		}
		printf("Received message: \n");
		break;
	}
	if (retry_count > MAX_RETRY)
	{
		printf("Server did not reply\n");
		close(sock);
	}
	else
	{
		if (n == 12)
		{
			calcMessage msg1;
			memcpy(&msg1, buf, sizeof(buf));
			if (msg1.type == 2 && msg1.message == 2 && msg1.major_version == 1 && msg1.minor_version == 0)
			{
				printf("The server sent a 'NOT OK' message\n");
				close(sock);
			}
		}
		else
		{
			memcpy(&res, buf, sizeof(buf));
			printf("id: %u\n", ntohl(res.id));
			printf("arith: %u\n", ntohl(res.arith));
			printf("inValue1: %u\n", ntohl(res.inValue1));
			printf("inValue2: %u\n", ntohl(res.inValue2));
			printf("inResult: %u\n", ntohl(res.inResult));
			printf("flValue1: %u\n", res.flValue1);
			printf("flValue2: %u\n", res.flValue2);
			// printf("flResult: %u\n", res.flResult);

			int arith = ntohl(res.arith);
			bool judge;
			double f1, f2, fresult;
			int i1, i2;
			uint32_t iresult;
			char op[10];
			char bufline[100];
			if (arith >= 5 & arith <= 8)
			{
				judge = 1;
				f1 = ntohl(res.flValue1);
				f2 = ntohl(res.flValue2);
			}
			else
			{
				judge = 0;
				i1 = ntohl(res.inValue1);
				i2 = ntohl(res.inValue2);
			}

			if (judge == 1)
			{
				// printf("(judge=%d)\n", judge);
				if (arith == 5)
				{
					fresult = f1 + f2;
				}
				else if (arith == 6)
				{
					fresult = f1 - f2;
				}
				else if (arith == 7)
				{
					fresult = f1 * f2;
				}
				else if (arith == 8)
				{
					fresult = f1 / f2;
				}
				// printf("%s %8.8g %8.8g = %8.8g\n", op, f1, f2, fresult);
				// printf("Calculated the result to %d\n", fresult);
				res.flResult = fresult;
				res.type = htons(2);
				int numbytes;
				// printf("type: %u\n", ntohs(res.type));
				printf("flResult: %u\n", res.flResult);

				int retry_count = 0;
				calcMessage msg_after;

#ifdef DEBUG
				while (retry_count <= MAX_RETRY)
				{
					// 发送消息
					printf("Sending message\n");
					numbytes = sendto(sock, &res, sizeof(res), 0, p->ai_addr, p->ai_addrlen);
					if (numbytes < 0)
					{
						perror("Failed to send message");
						exit(1);
					}

					// printf("sent %d bytes\n", numbytes);
					int n = recvfrom(sock, &msg_after, sizeof(msg_after), 0, p->ai_addr, &(p->ai_addrlen));
					if (n < 0)
					{
						printf("Timeout, retrying...\n");
						retry_count++;
						continue;
					}
					printf("Received message: \n");
					break;
				}
				if (retry_count > MAX_RETRY)
				{
					printf("Server did not reply\n");
					close(sock);
				}
				else
				{

					uint32_t re = ntohl(msg_after.message);
					if (re == 1)
					{
						printf("return: OK\n");
						printf("(mysesult=%d)\n", fresult);
					}
					else if (re == 0)
					{
						printf("Not applicable/availible\n");
					}
					else
					{
						printf("NOT OK\n");
					}
				}
			}
			else
			{
				// printf("(judge=%d)\n", judge);
				if (arith == 1)
				{
					iresult = i1 + i2;
				}
				else if (arith == 2)
				{
					iresult = i1 - i2;
				}
				else if (arith == 3)
				{
					iresult = i1 * i2;
				}
				else if (arith == 4)
				{
					iresult = i1 / i2;
				}
				else
				{
					printf("No match\n");
				}
				// printf("%s %d %d = %d \n",op,i1,i2,iresult);
				// printf("Calculated the result to %d\n", iresult);
				res.inResult = htonl(iresult);
				res.type = htons(2);
				int numbytes;
				// printf("type: %u\n", ntohs(res.type));
				printf("inResult: %u\n", ntohl(res.inResult));

				int retry_count = 0;
				calcMessage msg_after;

				while (retry_count <= MAX_RETRY)
				{
					// 发送消息
					printf("Sending message\n");
					numbytes = sendto(sock, &res, sizeof(res), 0,p->ai_addr, p->ai_addrlen);
					if (numbytes < 0)
					{
						perror("Failed to send message");
						exit(1);
					}

					// printf("sent %d bytes\n", numbytes);
					int n = recvfrom(sock, &msg_after, sizeof(msg_after), 0, p->ai_addr,&(p->ai_addrlen));
					if (n < 0)
					{
						printf("Timeout, retrying...\n");
						retry_count++;
						continue;
					}
					printf("Received message: \n");
					break;
				}
				if (retry_count > MAX_RETRY)
				{
					printf("Server did not reply\n");
					close(sock);
				}
				else
				{

					uint32_t re = ntohl(msg_after.message);
					if (re == 1)
					{
						printf("return: OK\n");
						printf("(mysesult=%d)\n", iresult);
					}
					else if (re == 0)
					{
						printf("Not applicable/availible\n");
					}
					else
					{
						printf("NOT OK\n");
					}
				}
			}
		}
	}
#endif
}
