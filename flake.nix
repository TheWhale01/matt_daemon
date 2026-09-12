{
  description = "Matt_daemon - A C++ daemon project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          name = "matt-daemon-dev-shell";

          # Utiliser clangStdenv pour que clang trouve les headers C
          stdenv = pkgs.clangStdenv;

          buildInputs = with pkgs; [
            # Compilateur C++ (gcc comme fallback)
            gcc
            gnumake

            # Outils de debug
            gdb
            valgrind
            bear

            # Outils de développement
            git
            clang-tools  # clang-format, clang-tidy
            clang  # Compilateur clang complet

            # Outils système UNIX
            coreutils
            util-linux

            # Documentation
            man-pages
            linux-manual
          ];

          shellHook = ''
            echo "🚀 Matt_daemon development environment loaded!"
            echo "📝 Available tools:"
            echo "   - g++ (C++ compiler)"
            echo "   - clang++ (C++ compiler)"
            echo "   - make (build tool)"
            echo "   - gdb (debugger)"
            echo "   - valgrind (memory checker)"
            echo "   - nc (netcat for testing)"
            echo "   - ss (socket statistics)"
            echo ""
            echo "💡 Quick start:"
            echo "   make          # Build the project"
            echo "   sudo ./Matt_daemon  # Run the daemon"
            echo "   nc localhost 4242    # Connect a client"
            echo ""
          '';
        };

        # Package optionnel si tu veux créer un binaire
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "matt-daemon";
          version = "1.0.0";

          src = self;

          buildInputs = with pkgs; [ gcc gnumake ];

          buildPhase = ''
            make
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp Matt_daemon $out/bin/
          '';
        };
      }
    );
}
