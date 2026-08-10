let
  apiVersion = builtins.head (
    builtins.match ".*RVMAPI_CURRENT_INTERFACE_VERSION ([0-9]+)u.*" (
      builtins.readFile ./include/RISCVModel/RVM.h
    )
  );
in

{
  stdenv,
  lib,
  cmake,
  doxygen,
  elfio,
}:

stdenv.mkDerivation {
  pname = "rvmi";
  version = "${apiVersion}";

  src = lib.fileset.toSource {
    root = ./.;
    fileset = lib.fileset.unions [
      ./CMakeLists.txt
      ./cmake
      ./docs
      ./include
      ./scripts
      ./src
      ./tests
    ];
  };

  outputs = [
    "out"
    "dev"
    "doc"
  ];

  cmakeFlags = [
    (lib.cmakeBool "BUILD_DOCS" true)
  ];

  # We only have a tiny static archive.
  outputLib = "dev";

  nativeBuildInputs = [
    cmake
    doxygen
  ];

  buildInputs = [
    elfio
  ];

  # No tests for now because no model impl to test against.
  doCheck = false;
  strictDeps = true;
}
