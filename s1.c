/*Saksham Mahajan - 2019B4A70627P*/

#include <stdio.h>  //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdbool.h>

#define LEN 512
#define PORT 8888
#define PDR 10 // define PDR in percentage i.e. if probability of dropping 10% then PDR = 10

typedef enum type_field
{
    ACK,
    DATA,
    FIN
} T;

typedef struct packet
{
    int size;
    int sq_no;
    char data[LEN];
    T type;
} PACKET;

int main(void)
{
    struct sockaddr_in s_door, sc1, sc2;
    srand(time(0)); // seed for random number rand()
    PACKET ack_pkt, rcv_pkt;
    int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        perror("socket()");
        exit(1);
    }
    printf("Server socket created successfully\n");

    memset((char *)&s_door, 0, sizeof(s_door));
    s_door.sin_family = AF_INET;
    s_door.sin_port = htons(PORT);
    s_door.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&s_door, sizeof(s_door)) == -1)
    {
        perror("bind");
        exit(1);
    }
    int newSocket, addressSize;
    printf("Binding done successfully\n");
    if (listen(sockfd, 5) < 0)
    {
        perror("listen()");
        exit(1);
    }
    printf("Start Listening\n");
    int one = accept(sockfd, (struct sockaddr *)&sc1, &addressSize);
    if (one < 0)
    {
        perror("accept()");
        exit(1);
    }
    printf("Connection accepted from c1 %s : %d\n", inet_ntoa(sc1.sin_addr), ntohs(sc1.sin_port));
    int two = accept(sockfd, (struct sockaddr *)&sc2, &addressSize);
    if (two < 0)
    {
        perror("accept()");
        exit(1);
    }
    printf("Connection accepted from c2 %s : %d\n", inet_ntoa(sc2.sin_addr), ntohs(sc2.sin_port));
    int state = 0;
    int c1_sq = 0;
    int c2_sq = 0;
    int c1_flag = 0;
    int c2_flag = 0;
    FILE *file_ptr = fopen("list.txt", "w");
    while (1)
    {
        switch (state)
        {
        case 0:
        {
            if (c1_flag == 1)
            {
                exit(0);
            }
            printf("Waiting for packet with seq no %d from sender c1 (%d)\n", c1_sq, ntohs(sc1.sin_port));
            recv(one, &rcv_pkt, sizeof(rcv_pkt), 0);
            int drop = rand() % ((int)(100 / PDR)) + 1; // Random number between 1 and PDR
            if (drop == 1)
            {
                printf("----Dropped packet with seq no: %d from client c1(%d)----\n", rcv_pkt.sq_no, ntohs(sc1.sin_port));
                continue; // Skip processing this packet
            }
            if (rcv_pkt.type == FIN)
            {
                c1_flag = 1;
                break;
            }
            if (rcv_pkt.sq_no == c1_sq)
            {
                printf("Packet received with seq. no. %d and packet content is = %s\n", rcv_pkt.sq_no, rcv_pkt.data);
                int counter = 0;
                while (counter < rcv_pkt.size)
                {
                    fputc(rcv_pkt.data[counter], file_ptr);
                    counter++;
                }
                fputc(',', file_ptr);
                ack_pkt.type = ACK;
                ack_pkt.sq_no = rcv_pkt.sq_no;
                send(one, &ack_pkt, sizeof(ack_pkt), 0);
                state = 1;
                c1_sq += rcv_pkt.size;
                break;
            }
            else
            {
                ack_pkt.sq_no = rcv_pkt.sq_no;
                send(one, &ack_pkt, sizeof(ack_pkt), 0);
            }
        }
        break;
        case 1:
        {
            if (c2_flag == 1)
            {
                exit(0);
            }
            printf("Waiting for packet with seq no %d from sender c2 (%d)\n", c2_sq, ntohs(sc2.sin_port));
            recv(two, &rcv_pkt, sizeof(rcv_pkt), 0);
            int drop = rand() % ((int)(100 / PDR)) + 1; // Random number between 1 and PDR
            if (drop == 1)
            {
                printf("----Dropped packet with seq no: %d from client c2(%d)----\n", rcv_pkt.sq_no, ntohs(sc1.sin_port));
                continue; // Skip processing this packet
            }
            if (rcv_pkt.type == FIN)
            {
                c2_flag = 1;
                break;
            }
            if (rcv_pkt.sq_no == c2_sq)
            {
                printf("Packet received with seq. no. %d and packet content is = %s\n", rcv_pkt.sq_no, rcv_pkt.data);
                int counter = 0;
                while (counter < rcv_pkt.size)
                {
                    fputc(rcv_pkt.data[counter], file_ptr);
                    counter++;
                }
                fputc(',', file_ptr);
                ack_pkt.type = ACK;
                ack_pkt.sq_no = rcv_pkt.sq_no;
                send(two, &ack_pkt, sizeof(ack_pkt), 0);
                state = 0;
                c2_sq += rcv_pkt.size;
                break;
            }
            else
            {
                ack_pkt.sq_no = rcv_pkt.sq_no;
                send(two, &ack_pkt, sizeof(ack_pkt), 0);
            }
        }
        break;
        }
        if (c1_flag == 1 || c2_flag == 1)
        {
            fseek(file_ptr, -1, SEEK_END);
            fputc(' ', file_ptr);
            break;
        }
        // }
    }

    fclose(file_ptr);
    close(sockfd);
}