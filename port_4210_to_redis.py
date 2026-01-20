import socket
import redis

UUT_PORT = 4210
REDIS_HOST = "10.12.196.123"
STREAM_KEY = "datastream"
STREAM_OBJ_KEY = "datas"
TRANSLEN = 20480
SAMPLE_SIZE = 40
CHUNK_SIZE = TRANSLEN * SAMPLE_SIZE


def run_producer():
    r = redis.Redis(host=REDIS_HOST, port=6379)

    buffer = bytearray(CHUNK_SIZE)
    print(f"buffer of len {len(buffer)} allocated")
    view = memoryview(buffer)

    print(f"Connecting to local data port {UUT_PORT}...")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("localhost", UUT_PORT))

        print("Streaming to Redis. Press Ctrl+C to stop.")
        try:
            while True:
                nbytes = sock.recv_into(view)
                if nbytes == 0:
                    break
                # Push to Redis Stream
                # maxlen ~2000 keeps the last ~2000 chunks in RAM to prevent memory overflow if the host stops.
                r.xadd(
                    STREAM_KEY,
                    {STREAM_OBJ_KEY: bytes(view[:nbytes])},
                    maxlen=2000,
                    approximate=True,
                )
        except KeyboardInterrupt:
            print("Stopped by user.")


if __name__ == "__main__":
    run_producer()
