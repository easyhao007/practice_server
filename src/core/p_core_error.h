//
// Created by root on 17-5-3.
//

#ifndef PRACTICE_SERVER_P_CORE_ERROR_H
#define PRACTICE_SERVER_P_CORE_ERROR_H

#include <stdio.h>
#include <string>
#include <time.h>
#include <vector>
#include <map>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#define trace(msg, ...) printf(msg"\n", ##__VA_ARGS__);
#define p_freep(p) delete p; p = NULL
#define p_assert(p) assert(p)
#define p_min(a, b) ((a)<(b)? (a):(b))

class p_core_error {

};


#endif //PRACTICE_SERVER_P_CORE_ERROR_H
