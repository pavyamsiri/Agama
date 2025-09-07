{
  description = "Agama development environment using uv2nix";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    pyproject-nix = {
      url = "github:pyproject-nix/pyproject.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    uv2nix = {
      url = "github:pyproject-nix/uv2nix";
      inputs.pyproject-nix.follows = "pyproject-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    pyproject-build-systems = {
      url = "github:pyproject-nix/build-system-pkgs";
      inputs.pyproject-nix.follows = "pyproject-nix";
      inputs.uv2nix.follows = "uv2nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, uv2nix, pyproject-nix, pyproject-build-systems, ... }:
    let
      inherit (nixpkgs) lib;

      # Load the uv workspace
      workspace = uv2nix.lib.workspace.loadWorkspace { workspaceRoot = ./.; };

      # Create package overlay from workspace
      overlay = workspace.mkPyprojectOverlay {
        sourcePreference = "wheel";
      };

      # Custom overrides for Agama-specific needs
      pyprojectOverrides = final: prev: {
        # Add any Agama-specific build fixups here
        # For example, if numpy needs BLAS/LAPACK:
        # "numpy" = prev."numpy".overrideAttrs (old: {
        #   buildInputs = (old.buildInputs or []) ++ [ final."openblas" ];
        #   propagatedBuildInputs = (old.propagatedBuildInputs or []) ++ [ final."openblas" ];
        # });
      };

      # System-specific configuration
      eachSystem = f: lib.genAttrs [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ] (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          python = pkgs.python3;  # Use default Python 3

          # Construct Python package set
          pythonSet = (pkgs.callPackage pyproject-nix.build.packages {
            inherit python;
          }).overrideScope (lib.composeManyExtensions [
            pyproject-build-systems.overlays.default
            overlay
            pyprojectOverrides
          ]);

        in f { inherit pkgs python pythonSet system; }
      );

    in {
      # Package for building Agama
      packages = eachSystem ({ pkgs, pythonSet, ... }: {
        default = pythonSet.agama or (throw "Agama package not found in Python set. Check your pyproject.toml name.");
      });

      # Development shells
      devShells = eachSystem ({ pkgs, python, pythonSet, system }: {
        # Impure shell: Uses uv directly with system Python
        impure = pkgs.mkShell {
          packages = with pkgs; [
            python
            pkgs.uv
            
            # System libraries and build tools
            gcc
            gfortran
            gnumake
            cmake
            pkg-config
            openblas
            lapack
            fftw
            gsl
            git
            doxygen
          ];

          env = {
            # uv configuration
            UV_PYTHON_DOWNLOADS = "never";
            UV_PYTHON = python.interpreter;
            
            # Compilation environment
            CFLAGS = "-I${pkgs.openblas}/include -I${pkgs.fftw.dev}/include -I${pkgs.gsl}/include";
            LDFLAGS = "-L${pkgs.openblas}/lib -L${pkgs.fftw}/lib -L${pkgs.gsl}/lib";
            BLAS = "${pkgs.openblas}/lib/libopenblas${pkgs.stdenv.hostPlatform.extensions.sharedLibrary}";
            LAPACK = "${pkgs.openblas}/lib/libopenblas${pkgs.stdenv.hostPlatform.extensions.sharedLibrary}";
          } // lib.optionalAttrs pkgs.stdenv.isLinux {
            LD_LIBRARY_PATH = lib.makeLibraryPath (with pkgs; [
              openblas
              fftw
              gsl
              stdenv.cc.cc.lib
            ]);
          };

          shellHook = ''
            unset PYTHONPATH
            echo "🔭 Agama impure development shell"
            echo "Using uv directly with system Python: $(python --version)"
            echo "System libraries provided by Nix: OpenBLAS, FFTW, GSL"
            echo ""
            echo "Run 'uv sync' to install Python dependencies"
            echo "Run 'make' to build C extensions"
          '';
        };

        # Pure shell: Uses uv2nix to construct a virtual environment from Nix
        uv2nix = let
          # Editable overlay for development
          editableOverlay = workspace.mkEditablePyprojectOverlay {
            root = "$REPO_ROOT";
          };

          # Python set with editable mode enabled
          editablePythonSet = pythonSet.overrideScope (lib.composeManyExtensions [
            editableOverlay
            
            # Agama-specific editable configuration
            (final: prev: {
              agama = prev.agama.overrideAttrs (old: {
                # Filter sources for editable build to avoid rebuilds on every change
                src = lib.cleanSourceWith {
                  src = old.src;
                  filter = path: type:
                    let
                      baseName = baseNameOf path;
                    in
                    baseName == "pyproject.toml" ||
                    baseName == "README.md" ||
                    lib.hasSuffix ".py" path ||
                    lib.hasSuffix ".c" path ||
                    lib.hasSuffix ".h" path ||
                    type == "directory";
                };

                # Add build system dependencies if needed
                nativeBuildInputs = (old.nativeBuildInputs or []) ++ final.resolveBuildSystem {
                  editables = [ ];
                };
              });
            })
          ]);

          # Virtual environment with editable packages
          virtualenv = editablePythonSet.mkVirtualEnv "agama-dev-env" workspace.deps.all;

        in pkgs.mkShell {
          packages = with pkgs; [
            virtualenv
            uv2nix.packages.${system}.uv
            
            # System libraries and build tools
            gcc
            gfortran
            gnumake
            cmake
            pkg-config
            openblas
            lapack
            fftw
            gsl
            git
            doxygen
          ];

          env = {
            # uv configuration
            UV_NO_SYNC = "1";
            UV_PYTHON = python.interpreter;
            UV_PYTHON_DOWNLOADS = "never";
            
            # Compilation environment
            CFLAGS = "-I${pkgs.openblas}/include -I${pkgs.fftw.dev}/include -I${pkgs.gsl}/include";
            LDFLAGS = "-L${pkgs.openblas}/lib -L${pkgs.fftw}/lib -L${pkgs.gsl}/lib";
            BLAS = "${pkgs.openblas}/lib/libopenblas${pkgs.stdenv.hostPlatform.extensions.sharedLibrary}";
            LAPACK = "${pkgs.openblas}/lib/libopenblas${pkgs.stdenv.hostPlatform.extensions.sharedLibrary}";
          };

          shellHook = ''
            unset PYTHONPATH
            export REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
            
            echo "🔭 Agama pure development shell (uv2nix)"
            echo "Using Nix-managed virtual environment with editable packages"
            echo "System libraries provided by Nix: OpenBLAS, FFTW, GSL"
            echo ""
            echo "Your Python environment is ready to use!"
            echo "Run 'python -c \"import agama\"' to test the package"
          '';
        };

        # Default shell (points to impure for familiarity)
        default = self.devShells.${system}.impure;
      });
    };
}
