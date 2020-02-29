#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

#define BUFLEN 50
#define TOPICLEN 50
#define IPLEN 32
#define IDLEN 16
#define STRINGLEN 1500
#define SIZEUDPRECV 1551

using namespace std;

struct __attribute__((packed)) send_udp_to_tcp {
	char udp_buf[SIZEUDPRECV];
	char ipbuf[IPLEN];
	int port;
	bool is_exit;
};

struct __attribute__((packed)) topic {

	char topic_name[TOPICLEN];
	int sf;
};

struct tcp_client {

    char id[IDLEN];
    bool connected;
	int sockfd;
	std::vector<topic> subs_topics;
	std::vector<send_udp_to_tcp> sf_msg;
};

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 3) {
		usage(argv[0]);
	}

	fd_set read_fds, tmp_fds;   // declar cele 2 fd seturi
   
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

    char id[BUFLEN];
    strcpy(id, argv[1]);
    n = send(sockfd, id, strlen(id), 0);    // trimit primul mesaj cu id-ul clientului
	DIE(n < 0, "send");

	FD_SET(STDIN_FILENO, &read_fds);    // adaug in read_fds standard inputul
    FD_SET(sockfd, &read_fds);      // adaug in read_fds socketul folosit pentru comunicatia cu serverul

	while (1) {

		tmp_fds = read_fds; // restore fds
       
        ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);   // multiplexez cu select
        // de aici in jos incep verificarile pe file descriptori	

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {     // daca am primit ceva de la stdin

			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			} else {
				if (strncmp(buffer, "subscribe", 9) == 0) {
					// se trimite mesaj la server
					n = send(sockfd, buffer, strlen(buffer), 0);
					DIE(n < 0, "send");
				}
				
				if (strncmp(buffer, "unsubscribe", 11) == 0) {
					// se trimite mesaj la server
					n = send(sockfd, buffer, strlen(buffer), 0);
					DIE(n < 0, "send");
				}

				// in caz de mesaj eronat ignor
			}
		}

		if (FD_ISSET(sockfd, &tmp_fds)) {	// daca am primit ceva de la server (de pe socketul sockfd)
            
			send_udp_to_tcp from_udp;

            n = recv(sockfd, (char*) &from_udp, sizeof(from_udp), 0);
            DIE(n < 0, "recv");

			char topic_rec[TOPICLEN];
			memcpy(topic_rec, from_udp.udp_buf, TOPICLEN);

			unsigned char type;
			memcpy(&type, from_udp.udp_buf + TOPICLEN, sizeof(uint8_t));

			uint32_t INT;
			uint16_t SHORT_REAL;
			uint32_t FLOAT;
			char STRING[STRINGLEN];
			uint8_t power;
			uint8_t sign;

			int vINT;
			float flSHORT_REAL;
			float flFLOAT;
			int t = type;

			if (from_udp.is_exit) {

                break;

            } else {
				switch (t) {
					case 0:
						memcpy(&sign, from_udp.udp_buf + TOPICLEN + 1, sizeof(uint8_t));
						memcpy(&INT, from_udp.udp_buf + TOPICLEN + 1 + sizeof(uint8_t), 
							sizeof(uint32_t));
								
						vINT = (sign == 0) ? (ntohl(INT)) : -(ntohl(INT));

						cout << from_udp.ipbuf << ":" << from_udp.port << " - " << topic_rec 
							<< " - INT - " << vINT << "\n";
						break;

					case 1:
						memcpy(&SHORT_REAL, from_udp.udp_buf + TOPICLEN + 1, sizeof(uint16_t));
						flSHORT_REAL = (float) (ntohs(SHORT_REAL) / 100.0f);

						cout << from_udp.ipbuf << ":" << from_udp.port << " - " << topic_rec 
							<< " - SHORT_REAL - " << flSHORT_REAL << "\n";
						break;

					case 2:
						memcpy(&sign, from_udp.udp_buf + TOPICLEN + 1, sizeof(uint8_t));
						memcpy(&FLOAT, from_udp.udp_buf + TOPICLEN + 1 + sizeof(uint8_t), 
							sizeof(uint32_t));
						memcpy(&power, from_udp.udp_buf + TOPICLEN + 1 + 
							sizeof(uint8_t) + sizeof(uint32_t), sizeof(uint8_t));

						flFLOAT = (sign == 0) ? (float) (ntohl(FLOAT) / pow(10, power))
							: -(float) (ntohl(FLOAT) / pow(10, power));

						cout << from_udp.ipbuf << ":" << from_udp.port << " - " << topic_rec 
							<< " - FLOAT - " << flFLOAT << "\n";
						break;

					case 3:
						memset(STRING, 0, STRINGLEN);
						memcpy(STRING, from_udp.udp_buf + TOPICLEN + 1, STRINGLEN);

						cout << from_udp.ipbuf << ":" << from_udp.port << " - " << topic_rec 
							<< " - STRING - " << STRING << "\n";
						break;

					default:
						cout << "Eroare mesaj udp\n";
				}
			}
		}
	}

	close(sockfd);

	return 0;
}