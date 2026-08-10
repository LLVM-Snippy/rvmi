# This is the cli entrypoint. Scope composition should just
# use the ./nix/components.nix as the scope function that gets
# merged downstream with all other components.

let
  sources = import ./nix/pins;
  lib = import (sources.nixpkgs + "/lib");
in

{
  stdenvAttrPath ? "stdenv",
  getStdenv ? pkgs: lib.getAttrFromPath (lib.splitString "." stdenvAttrPath) pkgs,
}:

let
  # TODO: We could instantiate nixpkgs with different cross systems if wanted.
  pkgs = import sources.nixpkgs { };
in

let
  # This produces the actual "scope" and is cross-compilation aware.
  # Hence why we do the splicing.
  mkScopeFor =
    pkgs:
    let
      # Can trivially mix in stdenvs to use in the scope.
      newScope = extra: pkgs.newScope (extra // { stdenv = getStdenv pkgs; });

      scope =
        lib.makeScopeWithSplicing'
          {
            inherit newScope;
            inherit (pkgs) splicePackages;
          }
          {
            # Magic incantation to create "splices" by instantiating with
            # different cross pkgs indices. This just calls mkScopeFor with all
            # the right splices of "pkgs...".
            otherSplices = lib.renameCrossIndexTo "self" (
              lib.mapCrossIndex (pkgs': (mkScopeFor pkgs')) (lib.renameCrossIndexFrom "pkgs" pkgs)
            );
            f = self: import ./nix/components.nix { inherit sources lib self; };
          };
    in
    scope;
in
(mkScopeFor pkgs) // { inherit mkScopeFor; }
