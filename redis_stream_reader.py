import redis
import zlib
import numpy as np
import matplotlib.pyplot as plt
import sys
import argparse
import matplotlib

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Fetch and plot compressed digitizer data from a Redis Stream.")
    parser.add_argument("stream_key", nargs="?", default="acq1102_056", help="The Redis stream key to query (default: acq1102_056)")
    parser.add_argument("--host", default="localhost", help="Redis server host (default: localhost)")
    parser.add_argument("--port", type=int, default=6379, help="Redis server port (default: 6379)")
    parser.add_argument("--plot-n-chans", type=int, default=1, help="Number of channels to plot (default: 1)")
    parser.add_argument("--compressed", type=int, default=0, help="Is channel compressed (default: 0)")
    parser.add_argument("--n-messages", type=int, default=1, help="Number of messages to retrieve (default: 1)")
    parser.add_argument("--payload-key", type=str, default="_", help="Payload key string (default: '_')")

    
    args = parser.parse_args()
    
    stream_key = args.stream_key
    host = args.host
    port = args.port
    plot_n_chans = args.plot_n_chans
    compressed = args.compressed
    n_messages = args.n_messages
    payload_key = bytes(args.payload_key, "utf-8")
    db = 0

    # decode_responses must be False for raw binary payloads
    try:
        r = redis.Redis(host=host, port=port, db=db, decode_responses=False)
        r.ping() # Test connection
    except redis.ConnectionError as e:
        print(f"Error connecting to Redis: {e}")
        sys.exit(1)

    print(f"Connected to Redis at {host}:{port}. Fetching latest record from '{stream_key}'...")

    # Fetch the single most recent message using XREVRANGE
    # Returns format: [(b'message_id', {b'field': b'value'})]
    messages = r.xrevrange(stream_key, count=n_messages)

    if not messages:
        print(f"Error: Stream '{stream_key}' is empty or does not exist.")
        sys.exit(1)

    # Unpack the message
    msg_id, msg_data = messages[0]
    
    # Extract the payload (keys are bytes because decode_responses=False)
    if payload_key not in msg_data:
        print(f"Error: Could not find '{payload_key}' field in the stream record.")
        sys.exit(1)
    
    if compressed:
        compressed_payload = msg_data[payload_key]
    
        # Decompression & Size Reporting
        try:
            decompressed_payload = zlib.decompress(compressed_payload)
        except zlib.error as e:
            print(f"Error decompressing payload: {e}")
            sys.exit(1)
    else:
        compressed_payload = msg_data[payload_key]
        decompressed_payload = msg_data[payload_key]
   
    # Save the raw decompressed binary to a file for external inspection
    bin_filename = f"{stream_key}_decompressed.bin"
    with open(bin_filename, "wb") as f:
        f.write(decompressed_payload)
    print(f"Saved raw decompressed binary data to: {bin_filename}")

    # print file stats
    comp_size = len(compressed_payload)
    decomp_size = len(decompressed_payload)
    print("-" * 40)
    print(f"Message ID        : {msg_id.decode('ascii')}")
    print(f"Compressed Size   : {comp_size} bytes")
    print(f"Decompressed Size : {decomp_size} bytes")
    print(f"Compression Ratio : {comp_size / decomp_size:.2%}")
    print("-" * 40)

    # Numpy Conversion & Reshaping
    # Convert raw bytes directly to a 1D array of int16
    data_1d = np.frombuffer(decompressed_payload, dtype=np.int16)
    
    # Ensure the data cleanly divides into 16 channels
    if data_1d.size % 16 != 0:
        print(f"Warning: Data size ({data_1d.size} int16 values) is not perfectly divisible by 16 channels.")
        # Truncate any dangling bytes to allow reshaping
        remainder = data_1d.size % 16
        data_1d = data_1d[:-remainder]

    # Reshape: -1 tells Numpy to automatically calculate the number of rows (samples)
    data_2d = data_1d.reshape(-1, 16)
    
    rows, cols = data_2d.shape
    print(f"NumPy Array Shape : {rows} Samples (Rows) x {cols} Channels (Columns)")
    print(f"Array Data Type   : {data_2d.dtype}")
    print("-" * 40)


    # Plotting
    # Safety check: Don't plot more channels than we actually have
    plot_n_chans = min(plot_n_chans, cols)
    print(f"Plotting {plot_n_chans} channel(s)...")
    
    for ch in range(plot_n_chans):
        # Select all rows for current column (channel)
        channel_data = data_2d[:, ch]
        
        plt.figure(figsize=(12, 6))
        plt.plot(channel_data, color='blue', alpha=0.8, linewidth=1.5)
        
        plt.title(f"Stream: {stream_key} | Channel {ch}")
        plt.xlabel("Sample Index")
        plt.ylabel("Amplitude (int16)")
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        
        # Save the plot to a file so it can be viewed in headless environments
        output_filename = f"{stream_key}_channel_{ch}.png"
        plt.savefig(output_filename)
        print(f"Plot successfully saved to: {output_filename}")
    
    # Only attempt to show the popup if the backend is interactive
    # This will open all generated figures at once
    if matplotlib.get_backend().lower() != 'agg':
        plt.show()
