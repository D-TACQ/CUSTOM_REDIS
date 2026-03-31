#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <hiredis/hiredis.h>
#include <zlib.h>

#define REVID "0.1.0"

#define DEFAULT_REDIS_SERVER_PORT "6379"
#define DEFAULT_REDIS_SERVER_IP "127.0.0.1"
#define DEFAULT_HOSTNAME "defaulthost"
#define DEFAULT_ACQ_PORT "4210"

#define SSB 64
#define TRANSLEN 1024
#define CHUNK_SIZE (SSB * TRANSLEN) // 65536 bytes

// Function to compress and send data to Redis
void compress_and_send(redisContext *c, const unsigned char *raw_data, size_t raw_len) {
    // 1. Prepare for compression
    uLongf compressed_len = compressBound(raw_len);
    unsigned char *compressed_data = (unsigned char *)malloc(compressed_len);
    
    if (!compressed_data) {
        printf("Memory allocation failed!\n");
        return;
    }

    // 2. Compress the payload
    int z_result = compress(compressed_data, &compressed_len, raw_data, raw_len);
    if (z_result != Z_OK) {
        printf("Zlib compression failed with code: %d\n", z_result);
        free(compressed_data);
        return;
    }

    // 3. Send to Redis using XADD
    // %b requires two arguments: pointer to data and size_t length
    redisReply *xadd_reply = redisCommand(c, 
        "XADD ctest * payload %b", 
        compressed_data, (size_t)compressed_len
    );

    if (xadd_reply == NULL) {
        printf("Redis XADD failed: %s\n", c->errstr);
    } else {
        printf("Pushed %lu raw bytes (compressed to %lu bytes). Entry ID: %s\n", 
               (unsigned long)raw_len, (unsigned long)compressed_len, xadd_reply->str);
        freeReplyObject(xadd_reply);
    }

    free(compressed_data);
}

int main(int argc, char **argv) {
    // read in environment variables for:
    // hostname (key)
    // subkey
    char* redis_server_port_str = getenv("REDIS_PORT");
    int redis_server_port;
    if (redis_server_port_str == NULL) {
        redis_server_port = atoi(DEFAULT_REDIS_SERVER_PORT);
    }
    else {
        redis_server_port = atoi(redis_server_port_str);
    }
    char* redis_server_ip = getenv("REDIS_IP");
    if (redis_server_ip == NULL) {
        redis_server_ip = DEFAULT_REDIS_SERVER_IP;
    }
    char* hostname = getenv("HOSTNAME");
    if (hostname == NULL) {
        hostname = DEFAULT_HOSTNAME;
        printf("default hostname used as redis key: %s\n", hostname);
    }
    else {
        printf("hostname used as redis key: %s\n", hostname);
    }

    char* acq_port_str = getenv("ACQ_PORT");
    int acq_port;
    if (acq_port_str == NULL) {
        acq_port = atoi(DEFAULT_ACQ_PORT);
    }
    else {
        acq_port = atoi(acq_port_str);
    }


    printf("Hello, Zynq from Nix! %s \n", REVID);
    printf("Linked against zlib version %s\n", zlibVersion());

    // Set up Redis Connection (Remote)
    redisContext *c = redisConnect(redis_server_ip, redis_server_port);
    if (c == NULL || c->err) {
        if (c) {
            printf("Redis connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Redis connection error: can't allocate context\n");
        }
        return 1;
    }
    printf("Connected to Redis at %s:%d successfully.\n", redis_server_ip, redis_server_port);

    // Set up TCP Client (Localhost)
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        redisFree(c);
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(acq_port);

    // Point to localhost
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        redisFree(c);
        return -1;
    }

    printf("Connecting to localhost:%d...\n", acq_port);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed. Is the daemon running on port %d?\n", acq_port);
        redisFree(c);
        return -1;
    }
    
    
    printf("Connected to localhost:%d! Receiving data...\n", acq_port);

    // Receive Data Loop
    unsigned char buffer[CHUNK_SIZE];
    int bytes_in_buffer = 0;

    while (1) {
        // Read data into the remaining space in our buffer
        int bytes_read = recv(sock, 
                              buffer + bytes_in_buffer, 
                              CHUNK_SIZE - bytes_in_buffer, 0);
        
        if (bytes_read < 0) {
            perror("Recv error");
            break;
        } else if (bytes_read == 0) {
            printf("Local daemon disconnected.\n");
            break;
        }

        bytes_in_buffer += bytes_read;

        // Once the buffer is exactly full, compress and push
        if (bytes_in_buffer == CHUNK_SIZE) {
            compress_and_send(c, buffer, CHUNK_SIZE);
            bytes_in_buffer = 0; // Reset for the next chunk
        }
    }

    // Cleanup
    close(sock);
    redisFree(c);
    printf("Exiting gracefully.\n");
    return 0;
}
