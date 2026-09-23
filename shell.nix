let
    pkgs = import <nixpkgs> {};
    pico-sdk = pkgs.pico-sdk.override { withSubmodules = true; };
in pkgs.mkShell {
    packages = [
        pkgs.gcc-arm-embedded
        pkgs.cmake
        pkgs.gnumake
        pkgs.git
        pkgs.python314

        pkgs.picotool
        pico-sdk
    ];
    PICO_SDK_PATH = "${pico-sdk}/lib/pico-sdk";
}
