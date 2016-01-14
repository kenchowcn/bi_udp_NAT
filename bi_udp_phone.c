#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>

#define INFO printf

#define TIME_OUT_SEC    10
#define TIME_OUT_USEC   0

void die(int line, char *fun)
{
    printf("[LINE:%d][FUN:%s] ERRNO=%d\n", line, fun, errno);
    exit(1);
}

int initRecvSock(int port)
{
    int sock;
    struct sockaddr_in si_recv;

    if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die(__LINE__, "socket()");
    }

    memset((char *) &si_recv, 0, sizeof(si_recv));

    si_recv.sin_family = AF_INET;
    si_recv.sin_port = htons(port);
    si_recv.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (struct sockaddr*)&si_recv, sizeof(si_recv) ) == -1)
    {
        die(__LINE__, "bind()");
    }

    return sock;
}

int recvMsg(int sock, struct sockaddr_in *si_remote)
{
    char buf[20];
    int recv_len;
    int slen = sizeof(struct sockaddr_in);

    memset(buf, 0, sizeof(buf));

    INFO("[LINE:%d]Start Receiving ... \n", __LINE__);

	if ((recv_len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)si_remote, &slen)) == -1)
	{
		die(__LINE__, "recvfrom()");
	}

	INFO("Received packet from %s:%d\n", inet_ntoa(si_remote->sin_addr), ntohs(si_remote->sin_port));

	//now reply the client with the same data
	if (sendto(sock, buf, recv_len, 0, (struct sockaddr*)si_remote, slen) == -1)
	{
		die(__LINE__, "sendto()");
	}

    return 0;
}

int initSendSock()
{
    int sock;

    if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die(__LINE__, "socket()");
    }

    return sock;
}

int sendMsg(int sock, struct sockaddr_in *si_remote)
{
    char buf[20] = "Hello";
    int slen = sizeof(struct sockaddr_in);

    if (0 >= sock)
    {
        die(__LINE__, "fd error");
    }

    if (sendto(sock, buf, sizeof(buf), 0, (struct sockaddr*)si_remote, slen)==-1)
    {
        die(__LINE__, "sendto()");
    }

    memset(buf, 0, sizeof(buf));
    if (recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)si_remote, &slen) == -1)
    {
        die(__LINE__, "recvfrom()");
    }

    return 0;
}

int fillRemoteInfo(char *server_ip, int server_port, struct sockaddr_in *si_remote)
{
    si_remote->sin_family = AF_INET;
    si_remote->sin_port = htons(server_port);

    if (inet_aton(server_ip , &si_remote->sin_addr) == 0)
    {
        die(__LINE__, "inet_aton()");
    }

    return 0;
}

int main()
{
    fd_set rset;
    int nready, recv_len;
    struct timeval timeout = {0};
    struct sockaddr_in si_remote;

    int recv_sock = initRecvSock(9011);
    int send_sock = initSendSock();

    memset(&si_remote, 0, sizeof(struct sockaddr_in));
    fillRemoteInfo("45.78.29.98", 9011, &si_remote);

    INFO("[LINE:%d] Prepare Sending...\n",__LINE__);
    sendMsg(send_sock, &si_remote);

    FD_ZERO(&rset);

    while(1)
    {
        FD_SET(recv_sock, &rset);
        timeout.tv_sec = TIME_OUT_SEC;
        timeout.tv_usec = TIME_OUT_USEC;

        nready = select(recv_sock+1, &rset, NULL, NULL, &timeout);
        if (nready == -1)
        {
            die(__LINE__, "select()");
        }
        else if (nready == 0)
        {
            if ((0 != si_remote.sin_port) && (0 != si_remote.sin_port))
            {
                INFO("[LINE:%d] Prepare Sending...\n",__LINE__);
                if (0 == sendMsg(send_sock, &si_remote))
                {
                    INFO("[LINE:%d]SendMsg Succeed\n", __LINE__);
                }
                else
                {
                    INFO("[LINE:%d]SendMsg Failed\n", __LINE__);
                }
            }
            continue;
        }
        else
        {
            if (FD_ISSET(recv_sock, &rset))
            {
                INFO("[LINE:%d] Prepare Receiving...\n",__LINE__);
                if (0 == recvMsg(recv_sock, &si_remote))
                {
                    INFO("[LINE:%d]recvMsg Succeed From %s:%d\n", __LINE__, inet_ntoa(si_remote.sin_addr), ntohs(si_remote.sin_port));
                }
                else
                {
                    INFO("[LINE:%d]recvMsg Failed\n", __LINE__);
                }
            }
        }
    }

}
