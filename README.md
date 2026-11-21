# Orbit Text

Native Resolume FFGL Source plugin for Windows x64.

## First build target

- Resolume Arena 7.x / FFGL 2.1+
- Three independent text inputs with individual speed and size
- Circle, rounded-square and rounded-rectangle paths
- RGB text color and adjustable glow
- Transparent output
- OpenGL rendering with a GDI-generated text mask

## Build

GitHub Actions builds the plugin on a Windows 2022 runner using the official [Resolume FFGL SDK](https://github.com/resolume/ffgl).

After a successful workflow run, download the **OrbitText-Windows-x64** artifact from the Actions run page. The DLL is not committed to this source repository.

## Current limitations of this first build

The first implementation uses Arial and a fixed 3-line layout. Font selection, letter-spacing, outline controls, user-selectable output dimensions and audio-reactive controls are planned for later iterations. The plugin must pass a successful CI build and be tested in Arena before it can be considered ready for live use.
