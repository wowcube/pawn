# Compilation
... using CLion.


## Windows
Visual Studio should be installed.

### Setup Toolchains
**File | Settings | Build, Execution, Development | Toolchains**

Add `Visual Studio` toolchain.

Set `amd64` architecture from dropdown list.

### Setup CMake
**File | Settings | Build, Execution, Development | CMake**

Create profile with `Build type: Release` and `Toolchain: Visual Studio`.

### Build
Build pawncc and pawnrun targets with `Release-Visual Studio` Run Configuration.

Following files will be in your build folder (e.g. `./cmake-build-release-visual-studio`):
- `amxArgs.dll`
- `amxDGram.dll`
- `amxFile.dll`
- `amxFixed.dll`
- `amxFloat.dll`
- `amxProcess.dll`
- `amxString.dll`
- `amxTime.dll`
- `pawncc.exe`
- `pawnrun.exe`

Copy them to WOWCube SDK bin directory.

## Linux
Can be built from WSL2 with WSL toolchain using GCC, **but do `chmod +x` on executables or use Linux to build**.

### Build
Build `pawncc` and `pawnrun` targets.

Following files will be in your build folder (e.g. `./cmake-build-release`):
- `amxArgs.so`
- `amxDGram.so`
- `amxFile.so`
- `amxFixed.so`
- `amxFloat.so`
- `amxProcess.so`
- `amxString.so`
- `amxTime.so`
- `pawnrun`
- `pawncc`

Copy them to WOWCube SDK bin directory:
```
mv -f pawncc pawnrun amx*.so ~/Projects/WOWCube_SDK/bin/pawn/linux
```

## macOS
`clang` should be installed.

### Build

Build `pawncc` and `pawnrun` targets.

Following files will be in your build folder (e.g. `./cmake-build-release`):
- `amxArgs.dylib`
- `amxDGram.dylib`
- `amxFile.dylib`
- `amxFixed.dylib`
- `amxFloat.dylib`
- `amxProcess.dylib`
- `amxString.dylib`
- `amxTime.dylib`
- `pawnrun`
- `pawncc`

Copy them to WOWCube SDK bin directory:
```
mv -f pawncc pawnrun amx*.dylib ~/IdeaProjects/WOWCube_SDK/bin/pawn/macos
```

Or try **Build | Install**.

Following files will be in your `/usr/local/bin` and `/usr/local/lib` folder:
- `/usr/local/bin/pawnrun`
- `/usr/local/bin/pawncc`
- `/usr/local/lib/amxArgs.dylib`
- `/usr/local/lib/amxDGram.dylib`
- `/usr/local/lib/amxFile.dylib`
- `/usr/local/lib/amxFixed.dylib`
- `/usr/local/lib/amxFloat.dylib`
- `/usr/local/lib/amxProcess.dylib`
- `/usr/local/lib/amxString.dylib`
- `/usr/local/lib/amxTime.dylib`

Copy them to WOWCube SDK bin directory:
```
mv -f /usr/local/bin/pawncc /usr/local/bin/pawnrun /usr/local/lib/amx*.dylib ~/Projects/WOWCube_SDK/bin/pawn/macos
```
