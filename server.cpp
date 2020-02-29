#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#define BUFLEN 50
#define TOPICLEN 50
#define IPLEN 32
#define IDLEN 16
#define MSGLEN 15
#define STRINGLEN 1500
#define SIZEUDPRECV 1551
#define MAX_CLIENTS 50

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
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	vector <tcp_client> clients; // retinem clientii abonati intr-un vector

	int sockfd_tcp, newsockfd_tcp, portno;
    int sockfd_udp;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0, "socket");

    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = bind(sockfd_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

    ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd_tcp, &read_fds);
    FD_SET(sockfd_udp, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

	fdmax = sockfd_tcp > sockfd_udp ? sockfd_tcp : sockfd_udp;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; ++i) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd_tcp = accept(sockfd_tcp, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd_tcp < 0, "accept");

                    char id_tcp[BUFLEN];
					n = recv(newsockfd_tcp, id_tcp, sizeof(id_tcp), 0);
					DIE(n < 0, "error receiving client TCP id\n");

					// se adauga noul socket intors de accept() la multimea descriptorilor 
                    // de citire
					FD_SET(newsockfd_tcp, &read_fds);
					if (newsockfd_tcp > fdmax) { 
						fdmax = newsockfd_tcp;
					}

                    cout << "New client " << id_tcp << " connected from " << 
                        inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << ".\n";

                    bool already_client = false;

                    for (int j = 0; j < clients.size(); ++j) {
                        if (strcmp(clients[j].id, id_tcp) == 0) {
                            already_client = true;
                        }
                    }
                    if (!already_client) {
                        tcp_client new_client;
					    new_client.sockfd = newsockfd_tcp; 
					    new_client.connected = true;
					    memcpy(new_client.id, id_tcp, IDLEN);
					    clients.push_back(new_client);
                    } else {
                        for (int j = 0; j < clients.size(); ++j) {
                            if (strcmp(clients[j].id, id_tcp) == 0) {
                                clients[j].sockfd = newsockfd_tcp;
                                clients[j].connected = true;
                                for (int k = 0; k < clients[j].sf_msg.size(); ++k) {
                                    ret = send(clients[j].sockfd, (char*) &clients[j].sf_msg[k], 
                                            sizeof(clients[j].sf_msg[k]), 0);
                                     DIE(ret < 0, "send");
                                }
                                clients[j].sf_msg.clear();
                                break;
                            }
					    }
                    }

                } else if (i == sockfd_udp) {

					send_udp_to_tcp msg_to_tcp;

                    struct sockaddr_in udp_rec;
                    socklen_t len_rec;
                    char udp_rec_str[SIZEUDPRECV];
					memset(udp_rec_str, 0, SIZEUDPRECV);

                    ret = recvfrom(sockfd_udp, udp_rec_str, SIZEUDPRECV, 0, 
                        (struct sockaddr*) &udp_rec, &len_rec);

                    strcpy(msg_to_tcp.ipbuf, inet_ntoa(udp_rec.sin_addr));
                    msg_to_tcp.port = ntohs(udp_rec.sin_port);
					memcpy(msg_to_tcp.udp_buf, udp_rec_str, SIZEUDPRECV);

					char cur_topic[TOPICLEN];
					memcpy(cur_topic, udp_rec_str, TOPICLEN);
					topic local_new_topic;
					strcpy(local_new_topic.topic_name, cur_topic);
					msg_to_tcp.is_exit = false;

					for (int j = 0; j < clients.size(); ++j) {
						for (int k = 0; k < clients[j].subs_topics.size(); ++k) {
							if (strcmp(clients[j].subs_topics[k].topic_name, cur_topic) == 0) {
								if (clients[j].connected) {
									n = send(clients[j].sockfd, (char*) &msg_to_tcp, sizeof(msg_to_tcp), 0);
									DIE(n < 0, "send");
								} else if (clients[j].subs_topics[k].sf == 1) {
								clients[j].sf_msg.push_back(msg_to_tcp);
								}
							} 
						}
					}

					DIE(ret < 0, "server error");

                } else if (i == STDIN_FILENO) {
                    
					char tmp[MSGLEN];
					fgets(tmp, MSGLEN, stdin);

					if (strncmp(tmp, "exit", 4) == 0) {
                        send_udp_to_tcp exit_msg;
						exit_msg.is_exit = true;

						for (int j = 0; j < clients.size(); ++j) {
							if (clients[j].connected == true) {
								n = send(clients[j].sockfd, (char*) &exit_msg, sizeof(exit_msg), 0);
								DIE(n < 0, "send");
							}
						}
						close(sockfd_tcp);
						close(sockfd_udp);
						return 0;
					}

				} else {
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						for (int j = 0; j < clients.size(); ++j) {
							if (clients[j].sockfd == i){
								cout << "Client " << clients[j].id << " disconnected.\n";
								clients[j].connected = false;
								break;
							}
						}
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
						continue; // pt ca nu mai are rost sa trec mai jos, iterez in continuare
					}

                    topic new_topic;

                    char *token = strtok(buffer, " ");

                    if (strcmp(token, "subscribe") == 0) {
                        token = strtok(NULL, " ");

						char topic_aux[TOPICLEN];
						strcpy(topic_aux, token);

                        strcpy(new_topic.topic_name, token);
                        token = strtok(NULL, " ");
					    new_topic.sf = atoi(token);

                        bool is_subscribed = false;
                        for (int j = 0; j < clients.size(); ++j) {
                            if (clients[j].sockfd == i) {
                                for (int k = 0; k < clients[j].subs_topics.size(); ++k) {
                                    if (strcmp(new_topic.topic_name, clients[j].subs_topics[k].topic_name) == 0) {
                                        is_subscribed = true;
                                        break;
                                    }
                                }
                            }
                        }

						// daca nu e deja abonat il bag in lista
                        if (!is_subscribed) {
                            for (int j = 0; j < clients.size(); ++j) {
                                if (clients[j].sockfd == i) {
                                    clients[j].subs_topics.push_back(new_topic);
                                    break;
                                } 
                            }
                        }

                    } else if(strcmp(token, "unsubscribe") == 0) {
                        token = strtok(NULL, " ");
                        for (int j = 0; j < clients.size(); ++j) {
                            if (clients[j].sockfd == i) {
                                for (int k = 0; k < clients[j].subs_topics.size(); ++k) {
                                    if (strncmp(clients[j].subs_topics[k].topic_name, token,
										strlen(clients[j].subs_topics[k].topic_name)) == 0) {
                                        clients[j].subs_topics.erase(clients[j].subs_topics.begin() + k);
                                        break;
                                    }
                                }
                            }
							 
						}
                    }
				}
			}
		}
	}

	close(sockfd_tcp);
	close(sockfd_udp);

	return 0;
}