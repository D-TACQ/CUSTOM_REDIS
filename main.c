#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <hiredis/hiredis.h>
#include <zlib.h>
#include <time.h>     // for clock_gettime, CLOCK_REALTIME
#include <stdint.h>   // for uint64_t
#include <inttypes.h> // for PRIu64
#include <libgen.h>  // for basename()

#define REVID "0.1.1"

#define DEFAULT_REDIS_SERVER_PORT "6379"
#define DEFAULT_REDIS_SERVER_IP "127.0.0.1"
#define DEFAULT_REDIS_MKEY "defaulthost"
#define DEFAULT_ACQ_PORT "4210"

#define SSB 64
#define TRANSLEN 1024
#define CHUNK_SIZE (SSB * TRANSLEN) // 65536 bytes

static int verbose;

void get_redis_stream_id(char *out_buf, size_t buf_len)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        exit(1);
    }

    // How many full milliseconds are hidden in the nanosecond part?
    uint64_t ms_from_nsec = (uint64_t)ts.tv_nsec/ 1000000ULL;

    // Total milliseconds since epoch and leftover ns
    uint32_t ns_within_ms = (uint32_t)ts.tv_nsec - ((uint32_t)ms_from_nsec * 1000000U);
    uint64_t milliseconds = ((uint64_t)ts.tv_sec * 1000ULL) + ms_from_nsec;

    // Print to string buffer in the <ms>-<ns> format
    // Use %06 to zero-pad the nanoseconds
    // to keep the stream IDs a consisten string length
    
    // snprintf(stream_id, 32, "%" PRIu64 "-%06" PRIu32 "\n", milliseconds, ns_within_ms);
    // Make sure you don't put a newline or carriage return at end of ID string!
    snprintf(out_buf, buf_len, "%" PRIu64 "-%06" PRIu32, milliseconds, ns_within_ms);
}

void print_redis_stream_id(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        exit(1);
    }

    // How many full milliseconds are hidden in the nanosecond part?
    uint64_t ms_from_nsec = (uint64_t)ts.tv_nsec/ 1000000ULL;

    // Total milliseconds since epoch and leftover ns
    uint32_t ns_within_ms = (uint32_t)ts.tv_nsec - ((uint32_t)ms_from_nsec * 1000000U);
    uint64_t milliseconds = ((uint64_t)ts.tv_sec * 1000ULL) + ms_from_nsec;

    // Print to string buffer in the <ms>-<ns> format
    // Use %06 to zero-pad the nanoseconds
    // to keep the stream IDs a consisten string length
    
    // snprintf(stream_id, 32, "%" PRIu64 "-%06" PRIu32 "\n", milliseconds, ns_within_ms);
    // Make sure you don't put a newline or carriage return at end of ID string!
    fprintf(stderr, "%" PRIu64 "-%06" PRIu32 "\n", milliseconds, ns_within_ms);
}
// Function to compress and send data to Redis
void compress_and_send(redisContext *c, char* redis_key, const unsigned char *raw_data, size_t raw_len) {
    if (redis_key == NULL || strlen(redis_key) == 0) {
        fprintf(stderr, "Error: Redis key is NULL or empty.\n");
        return;
    }
    if (strlen(redis_key) > 256) {
        fprintf(stderr, "Error: Redis key is too long.\n");
        return;
    }


    // Prepare for compression
    uLongf compressed_len = compressBound(raw_len);
    unsigned char *compressed_data = (unsigned char *)malloc(compressed_len);
    
    if (!compressed_data) {
        fprintf(stderr, "Memory allocation failed!\n");
        return;
    }

    // Compress the payload
    int z_result = compress(compressed_data, &compressed_len, raw_data, raw_len);
    if (z_result != Z_OK) {
        fprintf(stderr, "Zlib compression failed with code: %d\n", z_result);
        free(compressed_data);
        return;
    }

    // Send to Redis using XADD
    // %b requires two arguments: pointer to data and size_t length
    redisReply *xadd_reply = redisCommand(c, 
        "XADD %s * _ %b",
        redis_key,
        compressed_data, (size_t)compressed_len
    );

    if (xadd_reply == NULL) {
        fprintf(stderr, "Redis XADD failed: %s\n", c->errstr);
    } else {
        if (verbose) fprintf(stderr, "Pushed %lu raw bytes (compressed to %lu bytes). Entry ID: %s\n", 
                             (unsigned long)raw_len, (unsigned long)compressed_len, xadd_reply->str);
        /* Add the new code to write to stdout here
         *
         */
        printf("%s OK\n", xadd_reply->str);
        fflush(stdout);
        freeReplyObject(xadd_reply);
    }

    free(compressed_data);
}
void uncompressed_send(redisContext *c, char* redis_key, 
        char* id_buf, size_t id_buf_len, 
        const unsigned char *raw_data, size_t raw_len) 
{
    if (redis_key == NULL || strlen(redis_key) == 0) {
        fprintf(stderr, "Error: Redis key is NULL or empty.\n");
    }
    if (strlen(redis_key) > 256) {
        fprintf(stderr, "Error: Redis key is too long.\n");
        return;
    }

    // Prepare for compression
    //uLongf compressed_len = compressBound(raw_len);
    //unsigned char *compressed_data = (unsigned char *)malloc(compressed_len);
    
    //if (!compressed_data) {
    //    fprintf(stderr, "Memory allocation failed!\n");
    //    return;
    //}

    // Compress the payload
    //int z_result = compress(compressed_data, &compressed_len, raw_data, raw_len);
    //if (z_result != Z_OK) {
    //    fprintf(stderr, "Zlib compression failed with code: %d\n", z_result);
    //    free(compressed_data);
    //    return;
    //}

    // Send to Redis using XADD
    // %b requires two arguments: pointer to data and size_t length
    
    //redisReply *xadd_reply = redisCommand(c, 
    //    "XADD %s * _ %b",
    //    redis_key,
    //    raw_data, raw_len
    //);
    get_redis_stream_id(id_buf, id_buf_len);
    redisReply *xadd_reply = redisCommand(c, 
        "XADD %s %s _ %b",
        redis_key,
        id_buf,
        raw_data, raw_len
    );
    if (xadd_reply == NULL) {
        fprintf(stderr, "Redis XADD failed: %s\n", c->errstr);
    } else {
        if (verbose) fprintf(stderr, "Pushed %lu raw bytes (no compression of %lu bytes). Supplied Entry ID: %s  Entry ID: %s\n",
                             (unsigned long)raw_len, (unsigned long)raw_len, id_buf, xadd_reply->str);
        /* Add the new code to write to stdout here
         *
         */
        printf("%s OK\n", xadd_reply->str);
        fflush(stdout);
        freeReplyObject(xadd_reply);
    }

    //free(compressed_data);
}


// TODO: how customer gets nanoseconds from their systems...
//static uint64_t nanoseconds_since_epoch()
//{
//  return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
//}

static uint64_t nanoseconds_since_epoch(void)
{
    struct timespec ts;
    // CLOCK_REALTIME gets the system-wide real-time clock (time since epoch)
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        // fallback or error handling if clock_gettime fails
        return 0;
    }
    // Convert seconds to nanoseconds and add remaining nanoseconds
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}


int main(int argc, char **argv) {

    char *progname = basename(argv[0]);
    if (strcmp(progname, "redis-acq400") == 0) {
        verbose = 0;
    }
    else {
        verbose = 1;
    }

    if (verbose)
        fprintf(stderr, "Verbose mode enabled\n");


    // test functions
    uint64_t ns = nanoseconds_since_epoch();
    fprintf(stderr, "Nanoseconds since epoch: %" PRIu64 "\n", ns);
    fprintf(stderr, "Nanoseconds since epoch llu : %llu\n", (unsigned long long)ns);
    print_redis_stream_id();
    // read in environment variables for:
    // REDIS_MKEY (key)
    // subkey
    
    char id_buffer[32];

    char* redis_server_port_str = getenv("REDIS_PORT");
    int redis_server_port;
    if (redis_server_port_str == NULL) {
        redis_server_port = atoi(DEFAULT_REDIS_SERVER_PORT);
    }
    else {
        redis_server_port = atoi(redis_server_port_str);
    }
    char* redis_server_ip = getenv("REDIS_HOST");
    if (redis_server_ip == NULL) {
        redis_server_ip = DEFAULT_REDIS_SERVER_IP;
    }
    char* mkey = getenv("REDIS_MKEY");
    if (mkey == NULL) {
        mkey = DEFAULT_REDIS_MKEY;
        fprintf(stderr, "default redis mkey used as redis key: %s\n", mkey);
    }
    else {
        fprintf(stderr, "mkey used as redis key: %s\n", mkey);
    }

    char* acq_port_str = getenv("ACQ_PORT");
    int acq_port;
    if (acq_port_str == NULL) {
        acq_port = atoi(DEFAULT_ACQ_PORT);
    }
    else {
        acq_port = atoi(acq_port_str);
    }
    char* compress_p = getenv("COMPRESS");
    bool compress;
    if (compress_p == NULL || strcmp(compress_p,"0") == 0) {
        compress = false;
    }
    else {
        compress = true;
    }


    fprintf(stderr, "Hello, Zynq from Nix! This is a test %s \n", REVID);
    fprintf(stderr, "Linked against zlib version %s\n", zlibVersion());

    // Set up Redis Connection (Remote)
    redisContext *c = redisConnect(redis_server_ip, redis_server_port);
    uint64_t ns2 = nanoseconds_since_epoch();
    fprintf(stderr, "Nanoseconds since epoch: %" PRIu64 "\n", ns2);
    print_redis_stream_id();
    if (c == NULL || c->err) {
        if (c) {
           fprintf(stderr, "Redis connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
           fprintf(stderr, "Redis connection error: can't allocate context\n");
        }
        return 1;
    }
    fprintf(stderr, "Connected to Redis at %s:%d successfully.\n", redis_server_ip, redis_server_port);

    // Set up TCP Client (Localhost)
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "\n Socket creation error \n");
        redisFree(c);
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(acq_port);

    // Point to localhost
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "\nInvalid address/ Address not supported \n");
        redisFree(c);
        return -1;
    }

    fprintf(stderr, "Connecting to localhost:%d...\n", acq_port);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "\nConnection Failed. Is the daemon running on port %d?\n", acq_port);
        redisFree(c);
        return -1;
    }
    
    
    fprintf(stderr, "Connected to localhost:%d! Receiving data...\n", acq_port);

    // Receive Data Loop
    unsigned char data_buffer[CHUNK_SIZE];
    int bytes_in_data_buffer = 0;

    while (1) {
        // Read data into the remaining space in our buffer
        int bytes_read = recv(sock, 
                              data_buffer + bytes_in_data_buffer, 
                              CHUNK_SIZE - bytes_in_data_buffer, 0);
        
        if (bytes_read < 0) {
            perror("Recv error");
            break;
        } else if (bytes_read == 0) {
            fprintf(stderr, "Local daemon disconnected.\n");
            break;
        }

        bytes_in_data_buffer += bytes_read;

        /*
        void uncompressed_send(redisContext *c, char* redis_key, 
                               char* id_buf, size_t id_buf_len, 
                               const unsigned char *raw_data, size_t raw_len) 
        */
        
        // Once the buffer is exactly full, compress and push
        if (bytes_in_data_buffer == CHUNK_SIZE) {
            if (compress) {
                compress_and_send(c, mkey,  data_buffer, CHUNK_SIZE);
            }
            else {
                get_redis_stream_id(id_buffer, sizeof(id_buffer));
                uncompressed_send(c, mkey, id_buffer, sizeof(id_buffer), data_buffer, CHUNK_SIZE);
            }
            bytes_in_data_buffer = 0; // Reset for the next chunk
        }
    }

    // Cleanup
    close(sock);
    redisFree(c);
    fprintf(stderr, "Exiting gracefully.\n");
    return 0;
}
