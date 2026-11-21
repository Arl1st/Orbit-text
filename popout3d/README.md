# POP-OUT 3D

Native FFGL effect prototype for Resolume Arena 7.x / Windows x64.

The first prototype creates a red/cyan anaglyph from a single 2D input using luminance-derived pseudo-depth. It is not true stereo reconstruction; the depth estimate is heuristic and works best with footage containing clear contrast and lighting separation.

Controls:
- 3D Depth — overall parallax amount
- Pop Out — additional foreground separation
- Depth Focus — luminance plane treated as neutral depth
- Cyan Channel — strength of cyan-eye contribution
- Edge Glow — subtle warm edge accent
- 3D Direction — horizontal disparity direction
- 3D Blend — mix between original and stereo image

Build with the Windows x64 GitHub Actions workflow. Download artifact POP-OUT-3D-Windows-x64 after a successful run. Test in Arena before live use.
