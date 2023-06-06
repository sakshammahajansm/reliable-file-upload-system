/*Saksham Mahajan - 2019B4A70627P*/

#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>

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
    struct sockaddr_in serverAddr;
    srand(time(0)); // seed for random number rand()
    int s;
    char buf[LEN];
    PACKET snd_pkt, rcv_pkt;
    if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("socket()");
        exit(1);
    }
    printf("Client 2 socket created successfully\n");
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);                   // You can change port number here
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Specify server's IP address here
    printf("Address assigned successfully\n");
    int c = connect(s, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (c < 0)
    {
        perror("connect()");
        exit(1);
    }
    printf("Connection established successfully\n");
    int state = 0;
    FILE *file_ptr = fopen("id.txt", "rb");
    if (file_ptr == NULL)
    {
        perror("fopen()");
        exit(1);
    }
    char ch;
    int lastByte = 0, TotalBytes = 0;
    while (1)
    {
        switch (state)
        {
        case 0:
        {
            int ByteSize = 0;
            memset(snd_pkt.data, '\0', LEN);
            while ((ch = getc(file_ptr)) != EOF)
            {
                if (ch == ',' || ch == '.')
                {
                    break;
                }
                snd_pkt.data[ByteSize] = ch;
                ByteSize++;
            }
            if (ch == EOF)
            {
                snd_pkt.size = ByteSize;
                snd_pkt.sq_no = TotalBytes;
                snd_pkt.type = FIN;
                send(s, &snd_pkt, sizeof(snd_pkt), 0);
                exit(0);
            }
            printf("Pkt Data: %s\n", snd_pkt.data);
            snd_pkt.size = ByteSize;
            snd_pkt.sq_no = TotalBytes;
            snd_pkt.type = DATA;
            TotalBytes += ByteSize;
            lastByte = ByteSize;
            int bytesSent = send(s, &snd_pkt, sizeof(snd_pkt), 0);
            if (bytesSent < 0)
            {
                perror("send()");
                exit(1);
            }
            printf("SENT PKT: Seq No %d,Size %d\n",snd_pkt.sq_no,snd_pkt.size);
            state = 1;
        }
        break;
        case 1:
        {
            fd_set listen_for;
            FD_ZERO(&listen_for);
            int n;
            struct timeval timer;
            timer.tv_sec = 2;
            timer.tv_usec = 0;
            FD_SET(s, &listen_for);
            if ((n = select(s + 1, &listen_for,
                            NULL, NULL, &timer)) < 0)
            {
                perror("select");
                exit(1);
            }
            if (n == 0)
            {
                // state = 1;
                printf("Timeout Occurs!!!\n");
                if (send(s, &snd_pkt, sizeof(snd_pkt), 0) < 0)
                {
                    perror("sendto()");
                    exit(1);
                }
                printf("RETRANSMIT PKT: Seq No %d,Size %d\n",snd_pkt.sq_no,snd_pkt.size);
                break;
            }
            if ((recv(s, &rcv_pkt, sizeof(rcv_pkt), 0)) < 0)
            {
                perror("recv()");
                exit(1);
            }
            int drop = rand() % ((int)(100 / PDR)) + 1; // Random number between 1 and PDR
            if (drop == 1)
            {
                printf("----Dropped ACK with seq no: %d----\n", rcv_pkt.sq_no);
                continue; // Skip processing this packet
            }
            if (rcv_pkt.sq_no == TotalBytes - lastByte && rcv_pkt.type == ACK)
            {
                printf("RCVD ACK: Seq No %d\n", rcv_pkt.sq_no);
                state = 0;
            }
            else
                break;
        }
        break;
        }
    }

    fclose(file_ptr);
    close(s);
    return 0;
}