#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <hiredis/hiredis.h>
#include <zlib.h>

#define REDIS_IP_ADDR "10.12.196.123"
#define ACQPORT 4210
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
    printf("Hello, Zynq from Nix!\n");
    printf("Linked against zlib version %s\n", zlibVersion());

    // --- 1. Set up Redis Connection (Remote) ---
    int redis_port = 6379;
    redisContext *c = redisConnect(REDIS_IP_ADDR, redis_port);
    if (c == NULL || c->err) {
        if (c) {
            printf("Redis connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Redis connection error: can't allocate context\n");
        }
        return 1;
    }
    printf("Connected to Redis at %s:%d successfully.\n", REDIS_IP_ADDR, redis_port);

    // --- 2. Set up TCP Client (Localhost) ---
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        redisFree(c);
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ACQPORT);

    // Point to localhost
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        redisFree(c);
        return -1;
    }

    printf("Connecting to localhost:%d...\n", ACQPORT);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed. Is the daemon running on port %d?\n", ACQPORT);
        redisFree(c);
        return -1;
    }
    
    
    printf("Connected to localhost:%d! Receiving data...\n", ACQPORT);

    // --- 3. Receive Data Loop ---
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