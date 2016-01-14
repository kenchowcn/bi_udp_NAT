#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>

#define INFO printf

#define TIME_OUT_SEC    12
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

	if ((recv_len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)si_remote, &slen)) == -1)
	{
		die(__LINE__, "recvfrom()");
	}

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

    if (sendto(sock, buf, sizeof(buf), 0 , (struct sockaddr*)si_remote, slen)==-1)
    {
        //die(__LINE__, "sendto()");
        INFO("[LINE:%d]SendTo Failed, Continue...\n", __LINE__);
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    if (recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)si_remote, &slen) == -1)
    {
        //die(__LINE__, "recvfrom()");
        INFO("[LINE:%d]RecvFrom Failed, Continue...\n", __LINE__);
        return -1;
    }

    return 0;
}

char *getCurTime(char *time_str)
{
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64], buf[64];
    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);

    //strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    strftime(tmbuf, sizeof tmbuf, "%H:%M:%S", nowtm);
    snprintf(buf, sizeof(buf), "%s.%06d", tmbuf, tv.tv_usec);

    memset(time_str, 0, sizeof(char)*64);
    strcpy(time_str, buf);

    return time_str;
}

int main()
{
    fd_set rset;
    char time_str[64];
    int nready, recv_len;
    struct timeval timeout = {0};
    struct sockaddr_in si_remote;

    int recv_sock = initRecvSock(9011);
    int send_sock = initSendSock();

    FD_ZERO(&rset);
    memset(&si_remote, 0, sizeof(struct sockaddr_in));

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
            INFO("[LINE:%d] [%s] Prepare Sending...\n",__LINE__, getCurTime(time_str));
            if (0 == sendMsg(send_sock, &si_remote))
            {
                INFO("[LINE:%d] [%s] SendMsg Succeed\n", __LINE__, getCurTime(time_str));
            }
            else
            {
                INFO("[LINE:%d]SendMsg Failed\n", __LINE__);
            }

            continue;
        }
        else
        {
            if (FD_ISSET(recv_sock, &rset))
            {
                INFO("[LINE:%d] [%s] Prepare Receiving...\n",__LINE__, getCurTime(time_str));
                if (0 == recvMsg(recv_sock, &si_remote))
                {
                    INFO("[LINE:%d] [%s] recvMsg Succeed From %s:%d\n", __LINE__, getCurTime(time_str), inet_ntoa(si_remote.sin_addr), ntohs(si_remote.sin_port));
                }
                else
                {
                    INFO("[LINE:%d]recvMsg Failed\n", __LINE__);
                }
            }
        }
    }

}
