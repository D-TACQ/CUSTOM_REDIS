import socket
import time
import redis

# Host PC's IP address
REDIS_HOST = "10.12.196.123"

try:
    print(f"Attempting to connect to Redis at {REDIS_HOST}...")
    r = redis.Redis(host=REDIS_HOST, port=6379, socket_timeout=5)

    # Test basic connectivity
    if r.ping():
        print("SUCCESS: Connected to Redis!")

    # Test writing a small piece of data
    timestamp = time.ctime()
    r.set("client_heartbeat", f"Last seen: {timestamp} from {socket.gethostname()}")
    print(f"SUCCESS: Wrote heartbeat to Redis at {timestamp}")

except Exception as e:
    print(f"ERROR: Could not connect. Reason: {e}")
    print("\nTroubleshooting tips:")
    print(f"1. Can you 'ping {REDIS_HOST}' from this box?")
    print(f"2. Is 'bind 0.0.0.0' set in /etc/redis-stack.conf on the host?")
    print(f"3. Is the firewall on the host blocking port 6379?")
