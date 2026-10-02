#include "raylib.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int ScreenWidth = 1280;
constexpr int ScreenHeight = 720;
constexpr float FloorY = 640.0f;
constexpr float FighterGroundY = FloorY - 46.0f;
constexpr float SuperChargeRequired = 100.0f;
constexpr float BlockDuration = 1.2f;

struct Fighter {
    std::string name;
    std::string assetFolder;
    Color color;
    float speed;
    float power;
};

enum class FighterAction {
    Idle,
    Walk,
    Jump,
    Crouch,
    Block,
    DirectAttack,
    ReverseAttack,
    UpperAttack,
    LowerAttack,
    SuperAttack,
    Hurt,
    Count
};

constexpr int FighterActionCount = static_cast<int>(FighterAction::Count);

struct AnimationClip {
    std::vector<Texture2D> frames;
    float frameDuration = 0.1f;
    bool loops = false;
};

using FighterAnimationSet = std::array<AnimationClip, FighterActionCount>;

struct Enemy {
    Vector2 position;
    float health;
    float attackTimer;
    float hue;
};

struct DamagePopup {
    Vector2 position;
    std::string text;
    Color color;
    float life;
    float rise;
    float size;
};

struct Resolution {
    int width;
    int height;
};

struct Arena {
    std::string name;
    std::string backgroundPath;
    std::string groundPath;
    Texture2D background;
    Texture2D ground;
};

enum class DisplayMode { Windowed, Borderless, Fullscreen };
enum class Screen { Menu, Select, ArenaSelect, Settings, Fight, Pause, GameOver, ExitConfirm };

const std::vector<Fighter> Fighters = {
    {"VOLT", "volt", Color{245, 83, 98, 255}, 290.0f, 1.0f},
    {"NOVA", "nova", Color{64, 196, 255, 255}, 250.0f, 1.25f},
    {"EMBER", "ember", Color{255, 166, 64, 255}, 320.0f, 0.85f}
};

const std::vector<Resolution> Resolutions = {
    {1280, 720},
    {1366, 768},
    {1600, 900},
    {1920, 1080}
};

std::vector<Arena> Arenas;
std::vector<FighterAnimationSet> FighterAnimations;
Texture2D MenuBackground{};
float CameraWorldX = 0.0f;
constexpr float CameraDeadZoneFraction = 0.18f;
constexpr float CameraFollowSpeed = 5.0f;

const char* GetActionFolder(FighterAction action) {
    switch (action) {
    case FighterAction::Idle: return "idle";
    case FighterAction::Walk: return "walk";
    case FighterAction::Jump: return "jump";
    case FighterAction::Crouch: return "crouch";
    case FighterAction::Block: return "block";
    case FighterAction::DirectAttack: return "direct_attack";
    case FighterAction::ReverseAttack: return "reverse_attack";
    case FighterAction::UpperAttack: return "upper_attack";
    case FighterAction::LowerAttack: return "lower_attack";
    case FighterAction::SuperAttack: return "super_attack";
    case FighterAction::Hurt: return "hurt";
    case FighterAction::Count: break;
    }
    return "idle";
}

bool IsLoopingAction(FighterAction action) {
    return action == FighterAction::Idle || action == FighterAction::Walk ||
        action == FighterAction::Crouch || action == FighterAction::Block;
}

float GetArenaGroundY(const Arena& arena) {
    (void)arena;
    return FighterGroundY;
}

float GetArenaGroundWidth(const Arena& arena) {
    if (arena.ground.id != 0) return static_cast<float>(arena.ground.width);
    return static_cast<float>(ScreenWidth);
}

float SmoothLerp(float current, float target, float factor) {
    return current + (target - current) * factor;
}

void DiscoverArenas() {
    const std::string applicationRoot = GetApplicationDirectory();
    const std::string roots[] = {
        applicationRoot + "/assets/arenas",
        applicationRoot + "/../assets/arenas",
        "assets/arenas"
    };
    FilePathList folders{};
    for (const std::string& candidate : roots) {
        folders = LoadDirectoryFilesEx(candidate.c_str(), "DIR", false);
        if (folders.count > 0) break;
    }
    for (unsigned int index = 0; index < folders.count; ++index) {
        const std::string folderPath = folders.paths[index];
        const std::string folderName = GetFileName(folderPath.c_str());
        Arenas.push_back({folderName, folderPath + "/background.png", folderPath + "/ground.png", {}, {}});
    }
    if (folders.count > 0) {
        UnloadDirectoryFiles(folders);
    }
    if (Arenas.empty()) {
        Arenas.push_back({"DEFAULT ARENA", "", "", {}, {}});
    }
}

void LoadArenaTextures() {
    for (Arena& arena : Arenas) {
        if (FileExists(arena.backgroundPath.c_str())) arena.background = LoadTexture(arena.backgroundPath.c_str());
        if (FileExists(arena.groundPath.c_str())) arena.ground = LoadTexture(arena.groundPath.c_str());
    }
    const std::string applicationRoot = GetApplicationDirectory();
    const std::string menuPaths[] = {
        applicationRoot + "/assets/menu/background.png",
        applicationRoot + "/../assets/menu/background.png",
        "assets/menu/background.png"
    };
    for (const std::string& menuPath : menuPaths) {
        if (FileExists(menuPath.c_str())) {
            MenuBackground = LoadTexture(menuPath.c_str());
            break;
        }
    }
}

void LoadFighterAnimations() {
    const std::string applicationRoot = GetApplicationDirectory();
    const std::string fighterRoots[] = {
        applicationRoot + "/assets/fighters",
        applicationRoot + "/../assets/fighters",
        "assets/fighters"
    };
    std::string root;
    for (const std::string& candidate : fighterRoots) {
        if (DirectoryExists(candidate.c_str())) {
            root = candidate;
            break;
        }
    }

    FighterAnimations.resize(Fighters.size());
    if (root.empty()) return;

    for (size_t fighterIndex = 0; fighterIndex < Fighters.size(); ++fighterIndex) {
        for (int actionIndex = 0; actionIndex < FighterActionCount; ++actionIndex) {
            const FighterAction action = static_cast<FighterAction>(actionIndex);
            const std::string actionPath = root + "/" + Fighters[fighterIndex].assetFolder + "/" + GetActionFolder(action);
            if (!DirectoryExists(actionPath.c_str())) continue;

            FilePathList files = LoadDirectoryFilesEx(actionPath.c_str(), ".png", false);
            std::vector<std::string> framePaths;
            framePaths.reserve(files.count);
            for (unsigned int fileIndex = 0; fileIndex < files.count; ++fileIndex) {
                framePaths.emplace_back(files.paths[fileIndex]);
            }
            UnloadDirectoryFiles(files);
            std::sort(framePaths.begin(), framePaths.end());

            AnimationClip& clip = FighterAnimations[fighterIndex][actionIndex];
            clip.frameDuration = 0.09f;
            clip.loops = IsLoopingAction(action);
            for (const std::string& framePath : framePaths) {
                clip.frames.push_back(LoadTexture(framePath.c_str()));
            }
        }
    }
}

void UnloadFighterAnimations() {
    for (FighterAnimationSet& fighterAnimations : FighterAnimations) {
        for (AnimationClip& clip : fighterAnimations) {
            for (Texture2D& frame : clip.frames) {
                if (frame.id != 0) UnloadTexture(frame);
            }
        }
    }
}

void UnloadArenaTextures() {
    for (Arena& arena : Arenas) {
        if (arena.background.id != 0) UnloadTexture(arena.background);
        if (arena.ground.id != 0) UnloadTexture(arena.ground);
    }
    if (MenuBackground.id != 0) UnloadTexture(MenuBackground);
}

const char* GetDisplayModeName(DisplayMode mode) {
    switch (mode) {
    case DisplayMode::Windowed: return "WINDOWED";
    case DisplayMode::Borderless: return "BORDERLESS WINDOW";
    case DisplayMode::Fullscreen: return "FULLSCREEN";
    }
    return "WINDOWED";
}

void ApplyDisplaySettings(DisplayMode mode, int resolutionIndex) {
    const Resolution& resolution = Resolutions[resolutionIndex];
    if (IsWindowFullscreen()) ToggleFullscreen();
    ClearWindowState(FLAG_WINDOW_UNDECORATED);

    if (mode == DisplayMode::Borderless) {
        const int monitor = GetCurrentMonitor();
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        SetWindowPosition(0, 0);
    } else if (mode == DisplayMode::Fullscreen) {
        SetWindowSize(resolution.width, resolution.height);
        SetWindowState(FLAG_FULLSCREEN_MODE);
    } else {
        SetWindowSize(resolution.width, resolution.height);
    }
}

int FindClosestResolutionIndex(int width, int height) {
    int bestIndex = 0;
    int bestDistance = INT_MAX;
    for (int index = 0; index < (int)Resolutions.size(); ++index) {
        const int distance = std::abs(Resolutions[index].width - width) + std::abs(Resolutions[index].height - height);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = index;
        }
    }
    return bestIndex;
}

void DrawTextCentered(const char* text, int y, int size, Color color) {
    DrawText(text, (ScreenWidth - MeasureText(text, size)) / 2, y, size, color);
}

void DrawBar(Rectangle bounds, float currentValue, float visualValue, Color fill, Color damageColor, const char* label) {
    DrawRectangleRec(bounds, Color{18, 22, 38, 255});

    const float clampedCurrent = std::clamp(currentValue, 0.0f, 1.0f);
    const float clampedVisual = std::clamp(visualValue, 0.0f, 1.0f);
    const int currentWidth = static_cast<int>(bounds.width * clampedCurrent);
    const int visualWidth = static_cast<int>(bounds.width * clampedVisual);

    const int damageStart = std::max(currentWidth, 0);
    const int damageLength = std::max(0, std::min(visualWidth, (int)bounds.width) - damageStart);
    if (visualValue > currentValue) {
        DrawRectangle((int)bounds.x + damageStart, (int)bounds.y, damageLength, (int)bounds.height, damageColor);
    }

    DrawRectangle((int)bounds.x, (int)bounds.y, currentWidth, (int)bounds.height, fill);
    DrawRectangleLinesEx(bounds, 2, Color{237, 241, 255, 150});
    DrawText(label, (int)bounds.x, (int)bounds.y - 24, 18, RAYWHITE);
}

void DrawBackdrop(float time, const Arena& arena, float viewLeft) {
    ClearBackground(Color{9, 12, 27, 255});
    if (arena.background.id != 0) {
        DrawTexturePro(arena.background, {0, 0, (float)arena.background.width, (float)arena.background.height}, {viewLeft, 0, ScreenWidth, ScreenHeight}, {0, 0}, 0, WHITE);
        return;
    }
    DrawCircle(viewLeft + 1040, 130, 180.0f + std::sin(time) * 8.0f, Color{23, 42, 75, 255});
    DrawCircle(viewLeft + 1040, 130, 120.0f + std::sin(time * 1.4f) * 5.0f, Color{34, 62, 101, 255});
    for (int x = 0; x < ScreenWidth; x += 64) {
        DrawLine(viewLeft + x, 0, viewLeft + x, ScreenHeight, Color{24, 31, 53, 100});
    }
    for (int y = 0; y < ScreenHeight; y += 64) {
        DrawLine(0, y, ScreenWidth, y, Color{24, 31, 53, 100});
    }
}

void DrawMenuBackdrop(float time) {
    if (MenuBackground.id != 0) {
        DrawTexturePro(MenuBackground, {0, 0, (float)MenuBackground.width, (float)MenuBackground.height}, {0, 0, ScreenWidth, ScreenHeight}, {0, 0}, 0, WHITE);
        return;
    }
    ClearBackground(Color{9, 12, 27, 255});
    DrawCircle(1040, 130, 180.0f + std::sin(time) * 8.0f, Color{23, 42, 75, 255});
    DrawCircle(1040, 130, 120.0f + std::sin(time * 1.4f) * 5.0f, Color{34, 62, 101, 255});
    for (int x = 0; x < ScreenWidth; x += 64) DrawLine(x, 0, x, ScreenHeight, Color{24, 31, 53, 100});
    for (int y = 0; y < ScreenHeight; y += 64) DrawLine(0, y, ScreenWidth, y, Color{24, 31, 53, 100});
}

void DrawArenaGround(const Arena& arena) {
    if (arena.ground.id != 0) {
        const float foregroundHeight = std::max(80.0f, (float)arena.ground.height);
        const float groundTop = FloorY - (foregroundHeight - 80.0f);
        DrawTexturePro(arena.ground, {0, 0, (float)arena.ground.width, (float)arena.ground.height}, {0, groundTop, (float)arena.ground.width, foregroundHeight}, {0, 0}, 0, WHITE);
    }
}

void DrawFighter(Vector2 position, Color color, bool facingRight, bool isEnemy = false, FighterAction action = FighterAction::Idle) {
    const float direction = facingRight ? 1.0f : -1.0f;
    const bool crouched = action == FighterAction::Crouch;
    const int headOffset = crouched ? -83 : -112;
    const int bodyTop = crouched ? -62 : -82;
    const int bodyHeight = crouched ? 54 : 80;
    const int armOffset = crouched ? -48 : -72;
    const int legsTop = crouched ? -8 : -2;
    const int legsHeight = crouched ? 54 : 48;
    DrawCircle((int)position.x, (int)position.y + headOffset, 31, color);
    DrawRectangle((int)position.x - 28, (int)position.y + bodyTop, 56, bodyHeight, color);
    DrawRectangle((int)(position.x - 40 * direction), (int)position.y + armOffset, 80, 13, color);
    DrawRectangle((int)position.x - 22, (int)position.y + legsTop, 16, legsHeight, color);
    DrawRectangle((int)position.x + 6, (int)position.y + legsTop, 16, legsHeight, color);
    DrawCircle((int)(position.x + 11 * direction), (int)position.y + headOffset - 8, 5, Color{245, 247, 255, 255});
    if (isEnemy) {
        DrawCircle((int)(position.x - 11 * direction), (int)position.y + headOffset - 8, 5, Color{245, 247, 255, 255});
    }

    if (!isEnemy && action == FighterAction::Block) {
        DrawRectangle((int)(position.x + direction * 48.0f) - 8, (int)position.y - 126, 16, 78, Color{115, 235, 178, 210});
    } else if (!isEnemy && action >= FighterAction::DirectAttack && action <= FighterAction::SuperAttack) {
        const Color slashColor = action == FighterAction::SuperAttack ? Color{255, 218, 92, 230} : Color{245, 247, 255, 210};
        const float reach = action == FighterAction::SuperAttack ? 180.0f : 92.0f;
        const Vector2 torso{position.x, position.y - 65.0f};
        if (action == FighterAction::ReverseAttack) {
            DrawLineEx(torso, {position.x - reach, position.y - 82.0f}, 9.0f, slashColor);
            DrawLineEx(torso, {position.x + reach, position.y - 82.0f}, 9.0f, slashColor);
        } else {
            float verticalOffset = 0.0f;
            if (action == FighterAction::UpperAttack) verticalOffset = -58.0f;
            if (action == FighterAction::LowerAttack) verticalOffset = 44.0f;
            DrawLineEx(torso, {position.x + direction * reach, position.y - 82.0f + verticalOffset}, action == FighterAction::SuperAttack ? 15.0f : 9.0f, slashColor);
        }
    }
}

void DrawAnimatedFighter(Vector2 position, Color color, bool facingRight, size_t fighterIndex, FighterAction action, float actionTime) {
    if (fighterIndex < FighterAnimations.size()) {
        const AnimationClip& clip = FighterAnimations[fighterIndex][static_cast<int>(action)];
        if (!clip.frames.empty()) {
            const float frameDuration = std::max(clip.frameDuration, 0.01f);
            int frameIndex = static_cast<int>(actionTime / frameDuration);
            if (clip.loops) frameIndex %= static_cast<int>(clip.frames.size());
            else frameIndex = std::min(frameIndex, static_cast<int>(clip.frames.size()) - 1);

            const Texture2D& frame = clip.frames[frameIndex];
            const float sourceWidth = facingRight ? (float)frame.width : -(float)frame.width;
            DrawTexturePro(frame, {facingRight ? 0.0f : (float)frame.width, 0, sourceWidth, (float)frame.height},
                {position.x - 90.0f, position.y - 174.0f, 180.0f, 220.0f}, {0, 0}, 0, WHITE);
            return;
        }
    }

    DrawFighter(position, color, facingRight, false, action);
}

void ResetFight(float groundY, float enemySpawnX, std::vector<Enemy>& enemies, std::vector<DamagePopup>& damagePopups, float& playerHealth, float& spawnTimer, float& attackCooldown) {
    enemies.clear();
    damagePopups.clear();
    playerHealth = 1.0f;
    spawnTimer = 2.0f;
    attackCooldown = 0.0f;
    enemies.push_back({{enemySpawnX, groundY}, 1.0f, 1.2f, 0.0f});
}

void SpawnDamagePopup(std::vector<DamagePopup>& damagePopups, Vector2 position, int damage, bool isPlayerDamage) {
    const float magnitude = std::clamp(static_cast<float>(damage) / 28.0f, 0.2f, 3.0f);
    const float size = 18.0f + magnitude * 18.0f;
    const unsigned char red = static_cast<unsigned char>(std::clamp(200.0f + magnitude * 35.0f, 200.0f, 255.0f));
    const unsigned char green = static_cast<unsigned char>(std::clamp(110.0f - magnitude * 30.0f, 40.0f, 140.0f));
    const unsigned char blue = static_cast<unsigned char>(std::clamp(70.0f - magnitude * 20.0f, 20.0f, 90.0f));
    const Color color = isPlayerDamage ? Color{255, 179, 72, 255} : Color{red, green, blue, 255};
    const std::string text = TextFormat("-%d", damage);
    damagePopups.push_back({position, text, color, 0.9f, 0.0f, size});
}
}

int main() {
    InitWindow(ScreenWidth, ScreenHeight, "NEON BRAWL");
    InitAudioDevice();
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    const int monitor = GetCurrentMonitor();
    const int monitorWidth = GetMonitorWidth(monitor);
    const int monitorHeight = GetMonitorHeight(monitor);
    SetWindowSize(monitorWidth, monitorHeight);
    SetWindowState(FLAG_FULLSCREEN_MODE);

    DiscoverArenas();
    LoadArenaTextures();
    LoadFighterAnimations();

    Screen screen = Screen::Menu;
    int menuChoice = 0;
    int pauseChoice = 0;
    int settingsFocus = 0;
    int selectedArena = 0;
    int resolutionIndex = FindClosestResolutionIndex(monitorWidth, monitorHeight);
    DisplayMode displayMode = DisplayMode::Fullscreen;
    int selectedFighter = 0;
    float playerHealth = 1.0f;
    float playerHealthVisual = 1.0f;
    float enemyHealthVisual = 1.0f;
    float superCharge = 0.0f;
    float superChargeVisual = 0.0f;
    float blockTimer = 0.0f;
    bool blockNeedsRelease = false;
    float spawnTimer = 2.0f;
    float attackCooldown = 0.0f;
    float score = 0.0f;
    float time = 0.0f;
    Vector2 playerPosition{260.0f, FighterGroundY};
    FighterAction playerAction = FighterAction::Idle;
    float playerActionTime = 0.0f;
    float attackActionTimer = 0.0f;
    bool playerFacingRight = true;
    bool playerJumping = false;
    bool playerCrouching = false;
    bool playerBlocking = false;
    float playerVerticalVelocity = 0.0f;
    std::vector<Enemy> enemies;
    std::vector<DamagePopup> damagePopups;

    while (!WindowShouldClose()) {
        const float delta = GetFrameTime();
        time += delta;

        if (screen == Screen::Menu) {
            if (IsKeyPressed(KEY_UP)) menuChoice = (menuChoice + 2) % 3;
            if (IsKeyPressed(KEY_DOWN)) menuChoice = (menuChoice + 1) % 3;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (menuChoice == 0) screen = Screen::Select;
                if (menuChoice == 1) screen = Screen::Settings;
                if (menuChoice == 2) screen = Screen::ExitConfirm;
            }
        } else if (screen == Screen::Select) {
            if (IsKeyPressed(KEY_LEFT)) selectedFighter = (selectedFighter + 2) % 3;
            if (IsKeyPressed(KEY_RIGHT)) selectedFighter = (selectedFighter + 1) % 3;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                score = 0.0f;
                screen = Screen::ArenaSelect;
            }
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::Menu;
        } else if (screen == Screen::ArenaSelect) {
            if (IsKeyPressed(KEY_LEFT)) selectedArena = (selectedArena + (int)Arenas.size() - 1) % (int)Arenas.size();
            if (IsKeyPressed(KEY_RIGHT)) selectedArena = (selectedArena + 1) % (int)Arenas.size();
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                const float groundY = GetArenaGroundY(Arenas[selectedArena]);
                const float arenaWidth = GetArenaGroundWidth(Arenas[selectedArena]);
                const float arenaCenter = arenaWidth * 0.5f;
                const float spawnOffset = std::min(180.0f, std::max(0.0f, arenaCenter - 80.0f));
                const float minWalkX = 80.0f;
                const float maxWalkX = std::max(minWalkX + 80.0f, arenaWidth - 80.0f);
                const float playerSpawnX = std::clamp(arenaCenter - spawnOffset, minWalkX, maxWalkX);
                const float enemySpawnX = std::clamp(arenaCenter + spawnOffset, minWalkX, maxWalkX);
                ResetFight(groundY, enemySpawnX, enemies, damagePopups, playerHealth, spawnTimer, attackCooldown);
                playerHealthVisual = 1.0f;
                enemyHealthVisual = 1.0f;
                superCharge = 0.0f;
                superChargeVisual = 0.0f;
                blockTimer = 0.0f;
                blockNeedsRelease = false;
                playerPosition = {playerSpawnX, groundY};
                playerAction = FighterAction::Idle;
                playerActionTime = 0.0f;
                attackActionTimer = 0.0f;
                CameraWorldX = std::clamp(arenaCenter - ScreenWidth * 0.5f, 0.0f, std::max(0.0f, arenaWidth - ScreenWidth));
                screen = Screen::Fight;
            }
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::Select;
        } else if (screen == Screen::Settings) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) settingsFocus = 1 - settingsFocus;
            if (settingsFocus == 0) {
                if (IsKeyPressed(KEY_LEFT)) {
                    displayMode = static_cast<DisplayMode>((static_cast<int>(displayMode) + 2) % 3);
                    ApplyDisplaySettings(displayMode, resolutionIndex);
                }
                if (IsKeyPressed(KEY_RIGHT)) {
                    displayMode = static_cast<DisplayMode>((static_cast<int>(displayMode) + 1) % 3);
                    ApplyDisplaySettings(displayMode, resolutionIndex);
                }
            } else {
                if (IsKeyPressed(KEY_LEFT)) resolutionIndex = (resolutionIndex + (int)Resolutions.size() - 1) % (int)Resolutions.size();
                if (IsKeyPressed(KEY_RIGHT)) resolutionIndex = (resolutionIndex + 1) % (int)Resolutions.size();
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) ApplyDisplaySettings(displayMode, resolutionIndex);
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) screen = Screen::Menu;
        } else if (screen == Screen::Fight) {
            const Fighter& player = Fighters[selectedFighter];
            const float groundY = GetArenaGroundY(Arenas[selectedArena]);
            const float arenaWidth = GetArenaGroundWidth(Arenas[selectedArena]);
            const float minWalkX = 80.0f;
            const float maxWalkX = std::max(minWalkX + 80.0f, arenaWidth - 80.0f);
            const bool moveLeft = IsKeyDown(KEY_A);
            const bool moveRight = IsKeyDown(KEY_D);
            const float moveDirection = (moveRight ? 1.0f : 0.0f) - (moveLeft ? 1.0f : 0.0f);
            if (moveDirection != 0.0f) {
                playerFacingRight = moveDirection > 0.0f;
                playerPosition.x = std::clamp(playerPosition.x + moveDirection * player.speed * delta, minWalkX, maxWalkX);
            }
            const bool blockKeyDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            const bool blockKeyPressed = IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT);
            if (!blockKeyDown) blockNeedsRelease = false;
            if (blockKeyPressed && !blockNeedsRelease && blockTimer <= 0.0f) {
                blockTimer = BlockDuration;
                blockNeedsRelease = true;
            }
            blockTimer = std::max(0.0f, blockTimer - delta);
            playerBlocking = blockTimer > 0.0f;
            playerCrouching = IsKeyDown(KEY_S);
            if (IsKeyPressed(KEY_W) && !playerJumping) {
                playerJumping = true;
                playerVerticalVelocity = -640.0f;
            }

            if (playerJumping) {
                playerPosition.y += playerVerticalVelocity * delta;
                playerVerticalVelocity += 1400.0f * delta;
                if (playerPosition.y >= groundY) {
                    playerPosition.y = groundY;
                    playerJumping = false;
                    playerVerticalVelocity = 0.0f;
                }
            } else {
                playerPosition.y = groundY;
            }

            attackActionTimer = std::max(0.0f, attackActionTimer - delta);
            if (attackActionTimer <= 0.0f) {
                FighterAction nextAction = FighterAction::Idle;
                if (playerBlocking) nextAction = FighterAction::Block;
                else if (playerCrouching) nextAction = FighterAction::Crouch;
                else if (playerJumping) nextAction = FighterAction::Jump;
                else if (moveDirection != 0.0f) nextAction = FighterAction::Walk;
                if (playerAction != nextAction) {
                    playerAction = nextAction;
                    playerActionTime = 0.0f;
                }
            }
            playerActionTime += delta;

            const float cameraMaxX = std::max(0.0f, arenaWidth - ScreenWidth);
            const float deadZoneHalfWidth = ScreenWidth * CameraDeadZoneFraction;
            const float leftCameraBoundary = CameraWorldX + ScreenWidth * 0.5f - deadZoneHalfWidth;
            const float rightCameraBoundary = CameraWorldX + ScreenWidth * 0.5f + deadZoneHalfWidth;
            float cameraTargetX = CameraWorldX;
            if (playerPosition.x < leftCameraBoundary) {
                cameraTargetX = playerPosition.x - (ScreenWidth * 0.5f - deadZoneHalfWidth);
            } else if (playerPosition.x > rightCameraBoundary) {
                cameraTargetX = playerPosition.x - (ScreenWidth * 0.5f + deadZoneHalfWidth);
            }
            const float cameraSmoothing = 1.0f - std::exp(-CameraFollowSpeed * delta);
            CameraWorldX = std::clamp(SmoothLerp(CameraWorldX, cameraTargetX, cameraSmoothing), 0.0f, cameraMaxX);

            attackCooldown = std::max(0.0f, attackCooldown - delta);
            spawnTimer -= delta;

            if (spawnTimer <= 0.0f) {
                const float edgeOffset = static_cast<float>(GetRandomValue(0, 120));
                const float spawnX = playerPosition.x < arenaWidth * 0.5f ? maxWalkX - edgeOffset : minWalkX + edgeOffset;
                enemies.push_back({{spawnX, groundY}, 1.0f, 0.8f + GetRandomValue(0, 80) / 100.0f, static_cast<float>(GetRandomValue(0, 360))});
                spawnTimer = std::max(1.5f, 4.5f - score / 180.0f);
            }

            auto handleAttack = [&](float damage, float forwardRange, float backwardRange, bool canHitBehind, bool closeAttack, bool superAttack, FighterAction action) {
                if (attackCooldown > 0.0f) return false;
                if (superAttack && superCharge < SuperChargeRequired) return false;

                attackCooldown = 0.34f;
                if (superAttack) superCharge = 0.0f;
                playerAction = action;
                playerActionTime = 0.0f;
                attackActionTimer = superAttack ? 0.58f : 0.36f;
                for (Enemy& enemy : enemies) {
                    const float relativeX = enemy.position.x - playerPosition.x;
                    const float facingDistance = relativeX * (playerFacingRight ? 1.0f : -1.0f);
                    const bool inFront = facingDistance >= 0.0f && facingDistance <= forwardRange;
                    const bool behind = canHitBehind && facingDistance < 0.0f && std::abs(facingDistance) <= backwardRange;
                    const bool nearEnough = !closeAttack || std::abs(relativeX) <= forwardRange;
                    const bool verticalMatch = !closeAttack || (playerJumping ? enemy.position.y >= playerPosition.y - 70.0f : std::abs(enemy.position.y - playerPosition.y) <= 90.0f);
                    if ((inFront || behind) && nearEnough && verticalMatch) {
                        const float oldHealth = enemy.health;
                        const float dealtDamage = std::min(enemy.health, damage * player.power);
                        enemy.health -= dealtDamage;
                        if (enemy.health < oldHealth) {
                            if (!superAttack) superCharge = std::min(SuperChargeRequired, superCharge + dealtDamage * 100.0f);
                            const int value = static_cast<int>(std::round(dealtDamage * 100.0f));
                            enemyHealthVisual = std::max(enemyHealthVisual, oldHealth);
                            SpawnDamagePopup(damagePopups, {enemy.position.x, enemy.position.y - 120.0f}, value, false);
                        }
                    }
                }
                return true;
            };

            const bool pressedLeft = IsKeyPressed(KEY_LEFT);
            const bool pressedRight = IsKeyPressed(KEY_RIGHT);
            const bool pressedUp = IsKeyPressed(KEY_UP);
            const bool pressedDown = IsKeyPressed(KEY_DOWN);
            const bool pressedSuper = IsKeyPressed(KEY_SPACE);

            if (pressedSuper) {
                handleAttack(0.75f, 300.0f, 0.0f, false, false, true, FighterAction::SuperAttack);
            } else if (pressedLeft || pressedRight) {
                const bool directHit = (playerFacingRight && pressedRight) || (!playerFacingRight && pressedLeft);
                if (directHit) handleAttack(0.22f, 190.0f, 0.0f, false, false, false, FighterAction::DirectAttack);
                else handleAttack(0.16f, 150.0f, 150.0f, true, false, false, FighterAction::ReverseAttack);
            } else if (pressedUp) {
                handleAttack(0.30f, 125.0f, 0.0f, false, true, false, FighterAction::UpperAttack);
            } else if (pressedDown) {
                handleAttack(playerJumping ? 0.30f : 0.16f, 125.0f, 0.0f, false, true, false, FighterAction::LowerAttack);
            }

            for (Enemy& enemy : enemies) {
                const float toPlayer = playerPosition.x - enemy.position.x;
                enemy.position.x += (toPlayer > 0 ? 1.0f : -1.0f) * (70.0f + score / 18.0f) * delta;
                enemy.position.x = std::clamp(enemy.position.x, minWalkX, maxWalkX);
                enemy.attackTimer -= delta;
                if (std::abs(toPlayer) < 100.0f && enemy.attackTimer <= 0.0f) {
                    const float damageTaken = playerBlocking ? 0.015f : 0.08f;
                    const float previousHealth = playerHealth;
                    const float actualDamage = std::min(previousHealth, damageTaken);
                    playerHealth -= actualDamage;
                    if (playerHealth < previousHealth) {
                        const int value = static_cast<int>(std::round(actualDamage * 100.0f));
                        superCharge = std::min(SuperChargeRequired, superCharge + actualDamage * 100.0f);
                        playerHealthVisual = std::max(playerHealthVisual, previousHealth);
                        SpawnDamagePopup(damagePopups, {playerPosition.x, playerPosition.y - 140.0f}, value, true);
                        if (!playerBlocking) {
                            playerAction = FighterAction::Hurt;
                            playerActionTime = 0.0f;
                            attackActionTimer = 0.24f;
                        }
                    }
                    enemy.attackTimer = 1.1f;
                }
            }
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [&](const Enemy& enemy) {
                if (enemy.health <= 0.0f) {
                    score += 100.0f;
                    return true;
                }
                return false;
            }), enemies.end());

            playerHealthVisual = SmoothLerp(playerHealthVisual, playerHealth, 0.14f);
            superChargeVisual = SmoothLerp(superChargeVisual, superCharge, 0.16f);
            const float enemyHealth = enemies.empty() ? 0.0f : enemies.front().health;
            enemyHealthVisual = SmoothLerp(enemyHealthVisual, enemyHealth, 0.14f);

            for (auto& popup : damagePopups) {
                popup.life -= delta;
                popup.position.y -= 36.0f * delta;
            }
            damagePopups.erase(std::remove_if(damagePopups.begin(), damagePopups.end(), [](const DamagePopup& popup) {
                return popup.life <= 0.0f;
            }), damagePopups.end());

            if (playerHealth <= 0.0f) screen = Screen::GameOver;
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::Pause;
        } else if (screen == Screen::Pause) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) pauseChoice = 1 - pauseChoice;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (pauseChoice == 0) screen = Screen::Fight;
                if (pauseChoice == 1) screen = Screen::Menu;
            }
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::Fight;
        } else if (screen == Screen::GameOver) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) screen = Screen::Select;
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::Menu;
        } else if (screen == Screen::ExitConfirm) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                break;
            }
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::Menu;
        }

        BeginDrawing();
        ClearBackground(Color{9, 12, 27, 255});

        const float scale = std::min(
            static_cast<float>(GetScreenWidth()) / ScreenWidth,
            static_cast<float>(GetScreenHeight()) / ScreenHeight);
        const float viewLeft = (screen == Screen::Fight || screen == Screen::Pause) ? CameraWorldX : 0.0f;

        Camera2D virtualCamera{
            {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f},
            {viewLeft + ScreenWidth * 0.5f, ScreenHeight * 0.5f},
            0.0f,
            scale
        };
        BeginMode2D(virtualCamera);
        if (screen == Screen::Fight || screen == Screen::Pause) {
            DrawBackdrop(time, Arenas[selectedArena], viewLeft);
        } else {
            DrawMenuBackdrop(time);
        }

        if (screen == Screen::Menu) {
            DrawRectangle(370, 105, 540, 500, Color{9, 14, 30, 210});
            DrawRectangleLinesEx({370, 105, 540, 500}, 3, Color{64, 196, 255, 255});
            DrawTextCentered("NEON BRAWL", 155, 70, Color{245, 83, 98, 255});
            DrawTextCentered("2D ARENA // PROTOTYPE", 255, 22, Color{134, 153, 190, 255});
            const Color startColor = menuChoice == 0 ? Color{64, 196, 255, 255} : Color{70, 82, 116, 255};
            const Color settingsColor = menuChoice == 1 ? Color{64, 196, 255, 255} : Color{70, 82, 116, 255};
            const Color exitColor = menuChoice == 2 ? Color{245, 83, 98, 255} : Color{70, 82, 116, 255};
            DrawRectangle(390, 355, 500, 72, Color{28, 35, 62, 255});
            DrawRectangleLinesEx({390, 355, 500, 72}, 2, startColor);
            DrawTextCentered("START GAME", 377, 27, RAYWHITE);
            DrawRectangle(390, 430, 500, 72, Color{28, 35, 62, 255});
            DrawRectangleLinesEx({390, 430, 500, 72}, 2, settingsColor);
            DrawTextCentered("SETTINGS", 452, 27, RAYWHITE);
            DrawRectangle(390, 505, 500, 72, Color{28, 35, 62, 255});
            DrawRectangleLinesEx({390, 505, 500, 72}, 2, exitColor);
            DrawTextCentered("EXIT", 527, 27, RAYWHITE);
            DrawTextCentered("UP / DOWN select     ENTER confirm", 690, 18, Color{134, 153, 190, 255});
        } else if (screen == Screen::Select) {
            DrawTextCentered("CHOOSE YOUR FIGHTER", 70, 38, RAYWHITE);
            const Fighter& fighter = Fighters[selectedFighter];
            DrawRectangle(230, 145, 820, 380, Color{9, 14, 30, 210});
            DrawRectangleLinesEx({230, 145, 820, 380}, 3, Color{64, 196, 255, 255});
            DrawRectangle(250, 165, 380, 300, Color{19, 25, 45, 230});
            DrawRectangleLinesEx({250, 165, 380, 300}, 2, fighter.color);
            DrawFighter({440.0f, 415.0f}, fighter.color, true);
            DrawText("FIGHTER PROFILE", 680, 210, 18, Color{134, 153, 190, 255});
            DrawText(TextFormat("SPEED  %02d", (int)fighter.speed / 10), 680, 270, 24, Color{180, 192, 220, 255});
            DrawText(TextFormat("POWER  %02d", (int)(fighter.power * 100)), 680, 325, 24, Color{180, 192, 220, 255});
            DrawText("LEFT / RIGHT to browse", 680, 395, 17, Color{134, 153, 190, 255});
            DrawText(fighter.name.c_str(), 280, 475, 30, fighter.color);
            DrawTextCentered("LEFT / RIGHT select     ENTER confirm     ESC back", 690, 18, Color{134, 153, 190, 255});
        } else if (screen == Screen::ArenaSelect) {
            const Arena& arena = Arenas[selectedArena];
            DrawTextCentered("CHOOSE YOUR ARENA", 70, 42, RAYWHITE);
            DrawRectangle(230, 145, 820, 380, Color{19, 25, 45, 230});
            DrawRectangleLinesEx({230, 145, 820, 380}, 3, Color{64, 196, 255, 255});
            if (arena.background.id != 0) {
                DrawTexturePro(arena.background, {0, 0, (float)arena.background.width, (float)arena.background.height}, {250, 165, 780, 300}, {0, 0}, 0, WHITE);
            } else {
                DrawCircle(850, 245, 90, Color{34, 62, 101, 255});
                DrawText("PLACE BACKGROUND.PNG", 430, 305, 24, Color{134, 153, 190, 255});
                DrawText("IN THIS ARENA FOLDER", 470, 340, 20, Color{134, 153, 190, 255});
            }
            DrawText(arena.name.c_str(), 280, 475, 30, Color{64, 196, 255, 255});
            DrawText(TextFormat("ARENA %d / %d", selectedArena + 1, (int)Arenas.size()), 850, 480, 20, RAYWHITE);
            DrawTextCentered("LEFT / RIGHT select     ENTER confirm     ESC back", 690, 18, Color{134, 153, 190, 255});
        } else if (screen == Screen::Settings) {
            const Resolution& resolution = Resolutions[resolutionIndex];
            DrawTextCentered("DISPLAY SETTINGS", 80, 42, RAYWHITE);
            DrawRectangle(230, 145, 820, 380, Color{9, 14, 30, 210});
            DrawRectangleLinesEx({230, 145, 820, 380}, 3, Color{64, 196, 255, 255});
            DrawText("DISPLAY MODE", 300, 245, 22, Color{180, 192, 220, 255});
            DrawText(GetDisplayModeName(displayMode), 720, 245, 25, settingsFocus == 0 ? Color{64, 196, 255, 255} : RAYWHITE);
            DrawText("RESOLUTION", 300, 345, 22, Color{180, 192, 220, 255});
            DrawText(TextFormat("%d x %d", resolution.width, resolution.height), 720, 345, 25, settingsFocus == 1 ? Color{64, 196, 255, 255} : RAYWHITE);
            DrawTextCentered("UP / DOWN choose     LEFT / RIGHT change     ENTER / ESC back", 690, 18, Color{134, 153, 190, 255});
        } else if (screen == Screen::Fight || screen == Screen::Pause) {
            DrawLine(40, (int)FloorY, 1240, (int)FloorY, Color{64, 196, 255, 180});
            DrawAnimatedFighter(playerPosition, Fighters[selectedFighter].color, playerFacingRight, static_cast<size_t>(selectedFighter), playerAction, playerActionTime);
            for (const Enemy& enemy : enemies) DrawFighter(enemy.position, Color{156, 87, 211, 255}, false, true);
            for (const DamagePopup& popup : damagePopups) {
                DrawTextEx(GetFontDefault(), popup.text.c_str(), {popup.position.x - MeasureTextEx(GetFontDefault(), popup.text.c_str(), popup.size, 1.0f).x / 2.0f, popup.position.y}, popup.size, 1.0f, popup.color);
            }
            DrawArenaGround(Arenas[selectedArena]);

            DrawText("WAVE  01", (int)viewLeft + 44, 34, 24, Color{134, 153, 190, 255});
            DrawText(TextFormat("SCORE  %05d", (int)score), (int)viewLeft + 1020, 34, 24, RAYWHITE);
            DrawBar({viewLeft + 44, 80, 330, 18}, playerHealth, playerHealthVisual, Fighters[selectedFighter].color, Color{255, 126, 112, 255}, Fighters[selectedFighter].name.c_str());
            DrawText(TextFormat("SUPER  %d%%", (int)std::round(superCharge)), (int)viewLeft + 44, 108, 15, superCharge >= SuperChargeRequired ? Color{255, 218, 92, 255} : Color{180, 192, 220, 255});
            DrawRectangle((int)viewLeft + 44, 128, 330, 10, Color{18, 22, 38, 255});
            DrawRectangle((int)viewLeft + 44, 128, (int)(330.0f * std::clamp(superChargeVisual / SuperChargeRequired, 0.0f, 1.0f)), 10, superCharge >= SuperChargeRequired ? Color{255, 196, 64, 255} : Color{64, 196, 255, 255});
            DrawRectangleLinesEx({viewLeft + 44, 128, 330, 10}, 1, Color{237, 241, 255, 150});
            if (playerBlocking) DrawText("BLOCK ACTIVE", (int)viewLeft + 44, 146, 15, Color{115, 235, 178, 255});
            DrawBar({viewLeft + 906, 80, 330, 18}, enemies.empty() ? 0.0f : enemies.front().health, enemyHealthVisual, Color{245, 83, 98, 255}, Color{255, 166, 90, 255}, "INCOMING");
            DrawText("A/D move   W jump   S crouch   Shift block   Arrows attack   Space super", (int)viewLeft + 44, 662, 17, Color{134, 153, 190, 255});
            DrawText("ESC  menu", (int)viewLeft + 44, 684, 17, Color{134, 153, 190, 255});
            if (screen == Screen::Pause) {
                DrawRectangle((int)viewLeft, 0, ScreenWidth, ScreenHeight, Color{5, 8, 18, 170});
                DrawText("PAUSED", (int)viewLeft + (ScreenWidth - MeasureText("PAUSED", 42)) / 2, 70, 42, RAYWHITE);
                const Color continueColor = pauseChoice == 0 ? Color{64, 196, 255, 255} : Color{70, 82, 116, 255};
                const Color menuColor = pauseChoice == 1 ? Color{245, 83, 98, 255} : Color{70, 82, 116, 255};
                DrawRectangle((int)viewLeft + 370, 145, 540, 380, Color{9, 14, 30, 235});
                DrawRectangleLinesEx({viewLeft + 370, 145, 540, 380}, 3, Color{64, 196, 255, 255});
                DrawRectangle((int)viewLeft + 390, 245, 500, 72, Color{28, 35, 62, 245});
                DrawRectangleLinesEx({viewLeft + 390, 245, 500, 72}, 2, continueColor);
                DrawText("CONTINUE", (int)viewLeft + (ScreenWidth - MeasureText("CONTINUE", 27)) / 2, 267, 27, RAYWHITE);
                DrawRectangle((int)viewLeft + 390, 335, 500, 72, Color{28, 35, 62, 245});
                DrawRectangleLinesEx({viewLeft + 390, 335, 500, 72}, 2, menuColor);
                DrawText("MAIN MENU", (int)viewLeft + (ScreenWidth - MeasureText("MAIN MENU", 27)) / 2, 357, 27, RAYWHITE);
                DrawText("UP / DOWN select     ENTER confirm     ESC continue", (int)viewLeft + (ScreenWidth - MeasureText("UP / DOWN select     ENTER confirm     ESC continue", 18)) / 2, 690, 18, Color{180, 192, 220, 255});
            }
        } else if (screen == Screen::GameOver) {
            DrawTextCentered("SYSTEM FAILURE", 70, 42, Color{245, 83, 98, 255});
            DrawRectangle(230, 145, 820, 380, Color{9, 14, 30, 210});
            DrawRectangleLinesEx({230, 145, 820, 380}, 3, Color{64, 196, 255, 255});
            DrawTextCentered(TextFormat("FINAL SCORE  %05d", (int)score), 285, 28, RAYWHITE);
            DrawTextCentered("ENTER  choose another fighter", 425, 21, Color{134, 153, 190, 255});
            DrawTextCentered("ESC  main menu", 690, 18, Color{134, 153, 190, 255});
        } else if (screen == Screen::ExitConfirm) {
            DrawTextCentered("EXIT GAME?", 70, 42, Color{245, 83, 98, 255});
            DrawRectangle(230, 145, 820, 380, Color{9, 14, 30, 210});
            DrawRectangleLinesEx({230, 145, 820, 380}, 3, Color{64, 196, 255, 255});
            DrawTextCentered("Press ENTER to exit", 285, 25, RAYWHITE);
            DrawTextCentered("Press ESC to return to menu", 360, 21, Color{134, 153, 190, 255});
            DrawTextCentered("ENTER confirm     ESC back", 690, 18, Color{134, 153, 190, 255});
        }

        EndMode2D();
        EndDrawing();
    }

    CloseAudioDevice();
    UnloadFighterAnimations();
    UnloadArenaTextures();
    CloseWindow();
    return 0;
}
