## 🦃 Thanksgiving Theme — Beat Saber 1.45.3 development port

This branch is the separate Beat Saber **1.45.3** development port. The stable `main` branch and v1.0.0 release remain on Beat Saber 1.40.8.

The port preserves the existing golden harvest menu, harvest decorations, falling leaves, and the two turkey gobble audio cues. It updates the QPM dependencies, Android NDK, and QMOD target to the 1.45.3 Quest build (`1.45.3_30054`).

**Status:** Development build only. CI compilation and package validation are not a substitute for headset testing. Do not install this build over your stable 1.40.8 setup.

## Building

Requirements:
- QPM CLI
- Android NDK 29.0.14206865
- CMake 3.22+
- Ninja
- Python 3

```sh
qpm ndk resolve -d
qpm restore
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
qpm qmod zip --import mod.json --skip_build -i ./build -i ./extern/libs -i ./assets
```

The generated package targets Beat Saber `1.45.3_30054`. The stable 1.40.8 release is unchanged.
