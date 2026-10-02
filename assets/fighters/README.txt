Fighter sprite animation folders

Use assets/fighters/<fighter-id>/<action>/frame_000.png, frame_001.png, ...
Fighter IDs: volt, nova, ember.

Actions: idle, walk, jump, crouch, block, direct_attack, reverse_attack,
upper_attack, lower_attack, super_attack, hurt.

Frames are loaded in filename order. Use zero-padded frame numbers.
Recommended frame canvas: 180x220 pixels, transparent PNG (RGBA), with the
fighter's feet centered on the bottom edge. Frames are scaled to this canvas.
Missing action folders automatically use the procedural placeholder.
