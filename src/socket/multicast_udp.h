/*************************************************************************
	> File Name: multicast.h
	> Author: easyhao
	> Mail: easyhao007@163.com
	> Created Time: Wed 03 May 2017 10:15:06 AM CST
 ************************************************************************/

#ifndef _MULTICAST_H
#define _MULTICAST_H
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

using namespace std;

class multicast
{
public:
    multicast();
    virtual ~multicast();

private:
    //组播IP
    string  _multicast_ip;
    //本地网卡IP
    string  _local_ip;
    string  _port;
    //流ID
    int     _sid;

private:
    sockaddr_in     _addr;
    ip_merq         _merq;
    
public:
    int init();
    int start();
}
#endif
