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

You can also test the compiled binary on the host as qemu is installed and can run ARM binaries:
```
qemu-arm ./redis-acq400
```


Send the compiled binary to the UUT with:
```
scp result/bin/redis-acq400 root@$UUT:/mnt/local
```

