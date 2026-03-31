# redis acq400
## Compiling
This uses Nix to contain the cross-compilation environment.

The instructions are contained within the `flake.nix` file.

Install nix with:
```
curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix > nix-installer.sh
vim nix-installer.sh

# Then when you're happy with what it does
chmod +x nix-installer.sh
./nix-installer.sh install
```

Or if you trust the source completely then a shortcut is:
```
curl -fsSL https://install.determinate.systems/nix | sh -s -- install
```

Check the install went smoothly with:
```
nix --version
```

To build the project simply type:
```
nix build
```

You'll find the compiled binary at `result/bin/redis-acq400`.



To drop into an interactive development shell run:
```
nix develop
```
From here you can run traditional `make`, `make clean` commands to do things.

From the interactive development shell you can also test the compiled binary on the host as qemu is installed within it and can run ARM binaries:
```
qemu-arm ./redis-acq400
```


Send the compiled binary to the UUT with:
```
scp result/bin/redis-acq400 root@$UUT:/mnt/local
```
You can then run an in-situ test with:
```
/mnt/local/redis-acq400
```

You need to define the redis server IP address and port you are trying to connect to from the client.
This is done using environment variables whilst calling the program like:
```
HOSTNAME=$HOSTNAME REDIS_IP=10.12.196.123 REDIS_PORT=6379 ./redis-acq400
```
The `$HOSTNAME` is an existing shell variable but needs passed into the C program environment.

## redis-cli
If you have redis-cli installed you can connect to the redis server with:
```
redis-cli -h <host> -p <port>
```

If you don't supply a host or port number then it defaults to localhost and the default port (6379).

To get all possible keys:
```
KEYS pattern

127.0.0.1:6379> KEYS *
```

To show all values associated with a particular key (if they've been added with XADD):
```
XRANGE key start end [COUNT count]

127.0.0.1:6379> XRANGE keyname - +
```
