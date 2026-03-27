#include <stdio.h>
#include <zlib.h>

int main() {
    printf("Hello, Zynq from Nix!\n");
    printf("Linked against zlib version %s\n", zlibVersion());
    return 0;
}

