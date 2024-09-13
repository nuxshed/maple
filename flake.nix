{
  description = "nix flake for maple";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];

      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            clang-tools
            ncurses5
          ] ++ (if pkgs.stdenv.isDarwin then [] else [ pkgs.gdb ]);
        };
      });

      packages = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.stdenv.mkDerivation {
          pname = "maple";
          version = "0.1.0";

          src = self;
          buildInputs = [ pkgs.ncurses ];

          buildPhase = ''
            ${pkgs.clang}/bin/clang -o maple maple.c -lncurses
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp maple $out/bin/
          '';

          meta = with pkgs.lib; {
            description = "Maple - a simple terminal-based file explorer";
            license = licenses.mit;
            platforms = platforms.linux ++ platforms.darwin;
            maintainers = [ "nuxsh" ];
          };
        };
      });
    };
}
