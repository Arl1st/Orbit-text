# POP-OUT 3D v3

Native FFGL effect prototype for Resolume Arena 7.x / Windows x64.

Version 3 adds independent foreground and background disparity controls, a pop-out strength control, luminance depth-map mix, and depth inversion.

Controls:
- 3D Depth — overall parallax amount
- Pop Out — additional foreground separation
- Depth Focus — neutral luminance plane
- Cyan Channel — strength of cyan-eye contribution
- Edge Glow — subtle warm edge accent
- 3D Direction — horizontal disparity direction
- 3D Blend — mix between original and stereo image
- Foreground Depth — amount of positive/near depth
- Background Depth — amount of negative/far depth
- Pop Out Strength — multiplier for foreground protrusion
- Depth Map — blend from flat neutral depth to luminance-derived pseudo-depth
- Invert Depth — swaps near and far interpretation (0=off, 1=on)

Important: v3 still derives a pseudo-depth map from a single 2D image's luminance. It does not infer true object boundaries or reconstruct a real stereo pair. Results depend on lighting and contrast. Use moderate settings to avoid excessive image tearing.

Build with the Windows x64 GitHub Actions workflow. Download artifact POP-OUT-3D-Windows-x64 after a successful run. Test in Arena before live use.
