BUGS:
- If the block is transparent, always render the neighbouring blocks

VISUAL IMPROVEMENTS:
- Render water separately with a different shader
- Clouds

PERFORMANCE:
- Reserve the space for chunk mesh before calculating the mesh
- Debug memory leaks with rendering distance >= 16
