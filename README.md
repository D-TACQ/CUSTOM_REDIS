# CUSTOM_REDIS
## Compiling
This uses Nix to contain the cross-compilation environment.

The instructions that Nix used to build the Nix environment are are contained within the `flake.nix` file.


### Install nix
If you haven't already you will need to install nix to compile this project.

Install nix with the following commands:
```
# pull the installation script
curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix > nix-installer.sh

# inspect the script you are about to execute
vim nix-installer.sh

# Then when you're happy with what the script does
chmod +x nix-installer.sh
./nix-installer.sh install
```

Or if you trust the source of this bash script completely then a shortcut is:
```
curl -fsSL https://install.determinate.systems/nix | sh -s -- install
```

To check the install went smoothly, open a new terminal window and type the below command:
```
nix --version
```
You should see a version printout after running:

### Build the project
To build this project, from the top-level of the project directory structure type:
```
nix build
```

You'll then find the compiled binary at `result/bin/redis-acq400`.
The build artifact is a single static binary.

To drop into an interactive development shell run:
```
nix develop
```
From here you can run traditional `make`, `make clean` commands to do things.

From the interactive development shell you can also test the compiled binary on the host as qemu is installed within it and can run ARM binaries:
```
qemu-arm ./redis-acq400
```

### Deploy the package
Run `make.package` to create a tarball release under `release/`.

Use scp to send the file under `release/*.tar.gz` to the target UUT.

Reboot the UUT and the package will be loaded automatically.

## Running a test
If you'd like to run a test without deploying a package, then send the compiled binary to the UUT with:
```
scp result/bin/redis-acq400 root@$UUT:/mnt/local
```
You can then run an in-situ test with:
```
$ ssh root@$UUT
$ /mnt/local/redis-acq400
```

## configuring the program
You need to define the redis server IP address and port you are trying to connect to from the client.
This is done using environment variables whilst calling the program like:
```
HOSTNAME=$HOSTNAME REDIS_IP=10.12.196.123 REDIS_PORT=6379 COMPRESS=1 ./redis-acq400
```
The `$HOSTNAME` is an existing shell variable but needs passed into the C program environment.

The full range of environment variables that can be defined are:

REDIS_PORT The redis port (default 6379)
REDIS_HOST The redis hostname or IP address
REDIS_MKEY The redis mkey you want to the program to write to
ACQ_PORT   The port you want to get data from on ACQ400 device
COMPRESS   A flag for whether you want to zlib compress the data before forwarding over redis

## redis server
To run the redis server (so that your program has something to talk to) you will need to do some installation and configuration of another machine.
On a Linux machine of your choice that is network accessible, run:
```
sudo apt install redis-stack-server
redis-stack-server

sudo systemctl enable redis-server
sudo systemctl start redis-server

redis-cli
```

## redis-cli
If you have redis-cli installed you can connect to the redis server with:
```
redis-cli -h <host> -p <port>
```

If you don't explicitly supply a host or port number then it defaults to localhost and the default port (6379).

Once in a `redis-cli` shell there is a specialised command set that can be used to interact with the server.

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

For other commands see the redis-cli documentation.

## XRM EPICS IOC integration
The process of starting this program has been integrated with the XRM EPICS IOC.
Operating instructions TODO.

## python client
```
# You can call the environment anything you please but I've gone for env_redis_plot
python -m venv env_redis_plot
source env_redis_plot/bin/activate
pip install -r requirements.txt
python redis_stream_reader.py --help
redis_stream_reader.py --plot-n-chans 2 --compress 1




Redis client deployment on ACQ400 devices.

## Install on client
1. Clone repo
2. Run make.package
3. Deploy tarball from release folder over scp to /mnt/packages on ACQ400 device.

## Install a redis server on host
The client will need something to talk to.
Follow instructions here to install redis server on a Linux desktop:

https://redis.io/docs/latest/operate/oss_and_stack/install/archive/install-redis/install-redis-on-linux/

To install:
```
sudo apt-get install lsb-release curl gpg
curl -fsSL https://packages.redis.io/gpg | sudo gpg --dearmor -o /usr/share/keyrings/redis-archive-keyring.gpg
sudo chmod 644 /usr/share/keyrings/redis-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/redis-archive-keyring.gpg] https://packages.redis.io/deb $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/redis.list
sudo apt-get update
sudo apt-get install redis
```

Had to comment out all of the loadmodule commands in `/etc/redis-stack.conf` as below
```
$ sudo cat /etc/redis-stack.conf
port 6379
daemonize no
#loadmodule /opt/redis-stack/lib/rediscompat.so
#loadmodule /opt/redis-stack/lib/redisearch.so
#loadmodule /opt/redis-stack/lib/redistimeseries.so
#loadmodule /opt/redis-stack/lib/rejson.so
#loadmodule /opt/redis-stack/lib/redisbloom.so
#loadmodule /opt/redis-stack/lib/redisgears.so v8-plugin-path /opt/redis-stack/lib/libredisgears_v8_plugin.so

bind 0.0.0.0
```

Make sure redis port is open on firewall:
```
sudo ufw allow 6379/tcp
```

Tried to start with systemctl at first but no service was registered...
```                                    
sudo systemctl status redis-server
sudo cat /etc/redis-stack.conf
sudo systemctl start redis-server
sudo systemctl restart redis-server
sudo systemctl status redis-stack-server
sudo systemctl start redis-stack-server
which redis-stack-server
systemctl list-unit-files
systemctl list-unit-files | grep redis
```

So then started it up manually, using nohup to make it persist even if terminal is stopped:
```
sudo nohup /usr/bin/redis-stack-server /etc/redis-stack.conf &
```

## Initial test
Run the script `redis_test.py` on the ACQ400 device.
Run this command on the host:
```
$ redis-cli get client_heartbeat
```

You should see a timestamp and the client's hostname output.

## Monitoring 
Once both sides installed and running, you can monitor on host with this command:
```
# Shows operations per second and network payload
redis-cli --stat
```

Or use redis-cli commands directly
```
$ redis-cli
127.0.0.1:6379> 
127.0.0.1:6379> KEYS * 
1) "datastream"   
2) "client_heartbeat"
```
