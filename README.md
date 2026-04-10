# Slate

Slate is a persistent desktop canvas application built using C++ and Qt 6. It embeds directly into the Windows desktop layer, providing an opaque edge-to-edge drawing canvas while maintaining correct Z-order relative to desktop icons.

## Features

* **Persistent Desktop Integration:** Embeds as a layer above the Windows OS background.
* **Canvas Persistence:** Draws an infinite desktop canvas.
* **Non-Transparent UI:** Provides an opaque drawing interface.
* **Drag-and-Drop:** Support for dropping images and objects directly into the canvas.

## Prerequisites

To build Slate, you need:

* C++ compiler (e.g., MinGW or MSVC)
* CMake
* Qt 6

## Build Instructions (Windows)

The repository provides a script to easily run CMake and compile the project using MinGW.

1. Adjust the paths to your Qt and CMake installation inside `build.bat` if they differ from the defaults.
2. Run `build.bat` on the command line:

```bat
.\build.bat
```

3. The compiled executable `Slate.exe` along with Qt dependencies will be deployed in the `build/` directory using `windeployqt`.

## Running

Launch the executable from the `build` directory:
```bat
.\build\Slate.exe
```
