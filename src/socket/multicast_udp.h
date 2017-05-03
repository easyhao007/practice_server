/*************************************************************************
	> File Name: multicast.h
	> Author: easyhao
	> Mail: easyhao007@163.com
	> Created Time: Wed 03 May 2017 10:15:06 AM CST
 ************************************************************************/

#ifndef _MULTICAST_H
#define _MULTICAST_H
#include <p_core_error.h>

using namespace std;

class multicast
{
public:
    multicast(string local_ip , string m_ip , string m_port);
    virtual ~multicast();

private:
    //组播IP
    string  _multicast_ip;
    string  _multicast_port;
    //本地网卡IP
    string  _local_ip;
    //流ID
    int     _sid;

private:
    sockaddr_in     _addr;
    ip_mreq         _mreq;
    int             _fd;
public:
    //运行状态
    int             _status;
    string          _buffer;
public:
    int init();
    int start();
    int stop();
};
#endif
