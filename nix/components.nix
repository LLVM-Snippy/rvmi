{
  lib,
  sources,
  self, # This is the scope fixpoint.
  ...
}@args:

let
  inherit (self) callPackage;
in

# Here we can inject reverse dependencies that are useful for passthru.tests
# or something.
import ./scope.nix { inherit callPackage; }
