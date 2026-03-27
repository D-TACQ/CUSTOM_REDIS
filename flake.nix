{
  description = "Cross-compiled C program for Zynq-7000";
  inputs = {
    # We are pulling from unstable channel to get the latest tools
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      # The machine building on
      buildPlatform = "x86_64-linux";

      # Import standard package set for build machine
      pkgs = import nixpkgs { system = buildPlatform; };
      
      # Import cross-compilation package for 32-bit ARM
      crossPkgs = pkgs.pkgsCross.armv7l-hf-multiplatform.pkgsStatic;
    in {
      packages.${buildPlatform}.default = crossPkgs.stdenv.mkDerivation {
        pname = "zynq-hello";
        version = "0.1.0";

        # Source code is current directory
        src = ./.;

        # crossPkgs.stdenv automagically sets $CC to ARM cross-compiler
        buildPhase = ''
          $CC -o zynq-hello main.c
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp zynq-hello $out/bin/
        '';
      };
    };
}
