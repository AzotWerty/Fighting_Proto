NEON BRAWL - PORTABLE BUILD

Copy this entire folder to any Windows computer and double-click NeonBrawl.bat.
No CMake, compiler, MSYS2, source code, or internet connection is needed.

Controls:
A / D - walk
W - jump
S - crouch
Left / Right - direct and reverse attacks
Up / Down - upper and lower attacks
Shift - block
Space - super attack
Esc - pause / menu
Enter / Space - confirm in menus

Combat mechanics:
Super charge builds from damage dealt and received; use Space when the meter reaches 100%.
Block lasts 1.2 seconds, reduces most incoming damage, and requires releasing Shift before reactivation.
Direct and upper attacks hit forward; reverse attacks cover both sides; lower attacks deal more damage while jumping.

Fighter animation assets:
Add PNG frames under assets/fighters/<fighter-id>/<action>/frame_000.png.
IDs: volt, nova, ember. Actions: idle, walk, jump, crouch, block, direct_attack,
reverse_attack, upper_attack, lower_attack, super_attack, hurt.
Recommended frames are transparent RGBA PNGs, 180x220, feet centered at the bottom.
Missing animations automatically use procedural placeholders.

Fighting feedback:
Damage numbers appear as floating red/orange text when hits land.
The health bars shrink in real time, like a fighting game HUD.

Default display mode:
Fullscreen at the current monitor resolution.

Graphics compatibility:
This build uses OpenGL 2.1 for compatibility with older Windows 7 and virtual machines.
VirtualBox users should enable 3D acceleration in the VM display settings.

Assets:
assets/menu/background.png - main menu background
assets/arenas/<name>/background.png - arena background
assets/arenas/<name>/ground.png - arena ground, anchored to the bottom
Recommended ground size: 1280x80
