# CUSTOM_REDIS
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
