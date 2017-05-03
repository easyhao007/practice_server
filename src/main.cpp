#include <iostream>
#include <multicast_udp.h>
using namespace std;


int main(int argc , char** argv)
{
    multicast m_udp(argv[1] , argv[2] , argv[3]);
    m_udp.init();
    m_udp.start();
    m_udp.stop();

    return 0;
}
