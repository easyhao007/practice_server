/*************************************************************************
	> File Name: multicast.h
	> Author: easyhao
	> Mail: easyhao007@163.com
	> Created Time: Wed 03 May 2017 10:15:06 AM CST
 ************************************************************************/
#include "multicast_udp.h"

multicast::multicast(string local_ip , string multicast_ip , string multicast_port)
{
    _local_ip = local_ip;
    _multicast_ip = multicast_ip;
    _multicast_port = multicast_port;
}
multicast::~multicast(){ }


int multicast::init()
{
    int ret = 0;
    _fd = socket(AF_INET , SOCK_DGRAM , NULL);
    if(_fd < 0)
    {
        return -1;
    }

    /*uint8_t yes = 1;
    ret = setsockopt(_fd , SOL_SOCKET , SO_REUSEADDR , &yes , sizeof(yes));
    if(ret < 0)
    {
        return -1;
    }*/

    memset(&_addr , 0 , sizeof(_addr));
    _addr.sin_family = AF_INET;

    _addr.sin_addr.s_addr = htonl(INADDR_ANY);
    _addr.sin_port = htons(atoi(_multicast_port.c_str()));

    ret = bind(_fd , (sockaddr*)&_addr , sizeof(_addr));
    if(ret < 0)
    {
        return -1;
    }

    /*设置回环许可*/

    int loop = 1;
    ret = setsockopt(_fd , IPPROTO_IP , IP_MULTICAST_LOOP , &loop, sizeof(loop));

    if(ret < 0)
    {
        return -1;
    }
    _mreq.imr_multiaddr.s_addr=inet_addr(_multicast_ip.c_str());
    _mreq.imr_interface.s_addr=inet_addr(_local_ip.c_str());

    ret = setsockopt(_fd , IPPROTO_IP , IP_ADD_MEMBERSHIP , &_mreq , sizeof(_mreq));
    if(ret < 0)
    {
        return -1;
    }
    return ret;
}

int multicast::start()
{
    int ret = 0;
    
    if(_status == 1)
    {
        return ret;
    }

    _status = 1;

    while(_status)
    {
        int addr_len = sizeof(_addr);
        char buf[1316] = {0};
        int nbytes = recvfrom(_fd , buf , 1316 ,  0 , (sockaddr*)&_addr , (socklen_t*)&addr_len);
        if(nbytes < 0)
        {
            return -1;
        }
        printf("recv:%d bytes\n" , nbytes);
        _buffer += buf;
    }
    return ret;
}

int multicast::stop()
{
    int ret = 0;
    _status = 0;
    close(_fd);
    ret = setsockopt(_fd , IPPROTO_IP , IP_DROP_MEMBERSHIP,&_mreq , sizeof(_mreq));
    if(ret < 0)
    {
        return -1;
    }
    return ret;
}
