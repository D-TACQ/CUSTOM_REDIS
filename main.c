#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include <zlib.h>

#define IP_ADDR "10.12.196.123"
#define ACQPORT 4210

char* 
get_burst(int port) {
	char *burst = "Hello world from XADD burst!";
	return burst;
}

int main(int argc, char **argv) {
    printf("Hello, Zynq from Nix!\n");
    printf("Linked against zlib version %s\n", zlibVersion());
    printf("hello from redis client\n");
    redisReply *reply;
    redisReply *xadd_reply;
    redisContext *c;
    int redis_port = 6379;
    c = redisConnect(IP_ADDR, redis_port);
    if (c == NULL || c->err) {
            if (c) {
                    printf("Connection error on port %d: %s ", redis_port, c->errstr);
                    redisFree(c);
            } else {
                    printf("Connection error: can't allocate redis context");
            }
            return 1;
    }

    /* PING the server */
    printf("pinging the server...\n");
    reply = redisCommand(c, "PING %s", "Hello world");
    printf("RESPONSE: %s \n", reply->str);
    freeReplyObject(reply);

    /* try XADD */
    printf("trying xadd to the server...\n");
    char* stringy = get_burst(ACQPORT);
    xadd_reply = redisCommand(c, "XADD ctest 1234567890123-* message %s ", stringy);
    printf("RESPONSE: %s \n", xadd_reply->str);
    freeReplyObject(xadd_reply);
    redisFree(c);
    
    printf("goodbye from redis client\n");
    return 0;
}
