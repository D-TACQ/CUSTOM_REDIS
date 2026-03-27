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
        # Fetch and cross-compile zlib to static arm and expose headers/libs to your compiler
        buildInputs = [ crossPkgs.zlib ];

        # crossPkgs.stdenv automagically sets $CC to ARM cross-compiler
        buildPhase = ''
          $CC -o zynq-hello main.c -lz
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp zynq-hello $out/bin/
        '';
      };

      # definition of interactive development environment
      devShells.${buildPlatform}.default = crossPkgs.mkShell {
        # Pull in cross-compiler and zlib from package defined above
        inputsFrom = [ self.packages.${buildPlatform}.default ];

        # Add extra development tools here that aren't for final build
        # e.g. language server for editor
        nativeBuildInputs = [
          pkgs.clang-tools # provides clangd
          pkgs.file        # useful for checking compiled binary
        ];

        # A welcome message so we know we're in the right place
        shellHook = ''
          echo "Zynq Cross-Compilation Environment Loaded."
          echo "Compiler set to: $CC"
        '';
      };
    };
}
