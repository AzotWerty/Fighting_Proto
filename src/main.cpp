#include "raylib.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int ScreenWidth = 1280;
constexpr int ScreenHeight = 720;
constexpr float FloorY = 640.0f;
constexpr float FighterGroundY = FloorY - 46.0f;

struct Fighter {
    std::string name;
    Color color;
    float speed;
    float power;
};

struct Enemy {
    Vector2 position;
    float health;
    float attackTimer;
    float hue;
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
    {"VOLT", Color{245, 83, 98, 255}, 290.0f, 1.0f},
    {"NOVA", Color{64, 196, 255, 255}, 250.0f, 1.25f},
    {"EMBER", Color{255, 166, 64, 255}, 320.0f, 0.85f}
};

const std::vector<Resolution> Resolutions = {
    {1280, 720},
    {1366, 768},
    {1600, 900},
    {1920, 1080}
};

std::vector<Arena> Arenas;
Texture2D MenuBackground{};

float GetArenaGroundY(const Arena& arena) {
    (void)arena;
    return FighterGroundY;
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

void DrawBar(Rectangle bounds, float value, Color fill, const char* label) {
    DrawRectangleRec(bounds, Color{18, 22, 38, 255});
    DrawRectangle((int)bounds.x, (int)bounds.y, (int)(bounds.width * std::clamp(value, 0.0f, 1.0f)), (int)bounds.height, fill);
    DrawRectangleLinesEx(bounds, 2, Color{237, 241, 255, 150});
    DrawText(label, (int)bounds.x, (int)bounds.y - 24, 18, RAYWHITE);
}

void DrawBackdrop(float time, const Arena& arena) {
    ClearBackground(Color{9, 12, 27, 255});
    if (arena.background.id != 0) {
        DrawTexturePro(arena.background, {0, 0, (float)arena.background.width, (float)arena.background.height}, {0, 0, ScreenWidth, ScreenHeight}, {0, 0}, 0, WHITE);
        return;
    }
    DrawCircle(1040, 130, 180.0f + std::sin(time) * 8.0f, Color{23, 42, 75, 255});
    DrawCircle(1040, 130, 120.0f + std::sin(time * 1.4f) * 5.0f, Color{34, 62, 101, 255});
    for (int x = 0; x < ScreenWidth; x += 64) {
        DrawLine(x, 0, x, ScreenHeight, Color{24, 31, 53, 100});
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
        DrawTexturePro(arena.ground, {0, 0, (float)arena.ground.width, (float)arena.ground.height}, {0, groundTop, ScreenWidth, foregroundHeight}, {0, 0}, 0, WHITE);
    }
}

void DrawFighter(Vector2 position, Color color, bool facingRight, bool isEnemy = false) {
    const float direction = facingRight ? 1.0f : -1.0f;
    DrawCircle((int)position.x, (int)position.y - 112, 31, color);
    DrawRectangle((int)position.x - 28, (int)position.y - 82, 56, 80, color);
    DrawRectangle((int)(position.x - 40 * direction), (int)position.y - 72, 80, 13, color);
    DrawRectangle((int)position.x - 22, (int)position.y - 2, 16, 48, color);
    DrawRectangle((int)position.x + 6, (int)position.y - 2, 16, 48, color);
    DrawCircle((int)(position.x + 11 * direction), (int)position.y - 120, 5, Color{245, 247, 255, 255});
    if (isEnemy) {
        DrawCircle((int)(position.x - 11 * direction), (int)position.y - 120, 5, Color{245, 247, 255, 255});
    }
}

void ResetFight(float groundY, std::vector<Enemy>& enemies, float& playerHealth, float& spawnTimer, float& attackCooldown) {
    enemies.clear();
    playerHealth = 1.0f;
    spawnTimer = 2.0f;
    attackCooldown = 0.0f;
    enemies.push_back({{1020.0f, groundY}, 1.0f, 1.2f, 0.0f});
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

    Screen screen = Screen::Menu;
    int menuChoice = 0;
    int pauseChoice = 0;
    int settingsFocus = 0;
    int selectedArena = 0;
    int resolutionIndex = FindClosestResolutionIndex(monitorWidth, monitorHeight);
    DisplayMode displayMode = DisplayMode::Fullscreen;
    int selectedFighter = 0;
    float playerHealth = 1.0f;
    float spawnTimer = 2.0f;
    float attackCooldown = 0.0f;
    float score = 0.0f;
    float time = 0.0f;
    Vector2 playerPosition{260.0f, FighterGroundY};
    std::vector<Enemy> enemies;

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
                ResetFight(groundY, enemies, playerHealth, spawnTimer, attackCooldown);
                playerPosition = {260.0f, groundY};
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
            const float direction = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) ? -1.0f : (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT) ? 1.0f : 0.0f);
            playerPosition.x = std::clamp(playerPosition.x + direction * player.speed * delta, 80.0f, 1200.0f);
            attackCooldown = std::max(0.0f, attackCooldown - delta);
            spawnTimer -= delta;

            if (spawnTimer <= 0.0f) {
                const float spawnX = 760.0f + static_cast<float>(GetRandomValue(0, 400));
                enemies.push_back({{spawnX, groundY}, 1.0f, 0.8f + GetRandomValue(0, 80) / 100.0f, static_cast<float>(GetRandomValue(0, 360))});
                spawnTimer = std::max(1.5f, 4.5f - score / 180.0f);
            }

            const bool attacking = IsKeyDown(KEY_J) || IsKeyDown(KEY_K);
            if (attacking && attackCooldown <= 0.0f) {
                attackCooldown = 0.34f;
                const float damage = IsKeyDown(KEY_K) ? 0.32f : 0.2f;
                for (Enemy& enemy : enemies) {
                    if (std::abs(enemy.position.x - playerPosition.x) < 190.0f) enemy.health -= damage * player.power;
                }
            }

            for (Enemy& enemy : enemies) {
                const float toPlayer = playerPosition.x - enemy.position.x;
                enemy.position.x += (toPlayer > 0 ? 1.0f : -1.0f) * (70.0f + score / 18.0f) * delta;
                enemy.attackTimer -= delta;
                if (std::abs(toPlayer) < 100.0f && enemy.attackTimer <= 0.0f) {
                    playerHealth -= 0.08f;
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
        Camera2D virtualCamera{
            {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f},
            {ScreenWidth * 0.5f, ScreenHeight * 0.5f},
            0.0f,
            scale
        };
        BeginMode2D(virtualCamera);
        if (screen == Screen::Fight || screen == Screen::Pause) DrawBackdrop(time, Arenas[selectedArena]);
        else DrawMenuBackdrop(time);

        if (screen == Screen::Menu) {
            DrawTextCentered("NEON BRAWL", 150, 86, Color{245, 83, 98, 255});
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
            DrawTextCentered("UP / DOWN select     ENTER confirm", 625, 18, Color{134, 153, 190, 255});
        } else if (screen == Screen::Select) {
            DrawTextCentered("CHOOSE YOUR FIGHTER", 70, 38, RAYWHITE);
            for (int index = 0; index < 3; ++index) {
                const int x = 175 + index * 320;
                const bool selected = index == selectedFighter;
                DrawRectangle(x, 165, 240, 330, selected ? Color{30, 49, 78, 255} : Color{19, 25, 45, 255});
                DrawRectangleLinesEx({(float)x, 165, 240, 330}, selected ? 4 : 2, selected ? Fighters[index].color : Color{70, 82, 116, 255});
                DrawFighter({x + 120.0f, 415.0f}, Fighters[index].color, true);
                DrawText(Fighters[index].name.c_str(), x + 26, 205, 30, Fighters[index].color);
                DrawText(TextFormat("SPEED  %02d", (int)Fighters[index].speed / 10), x + 26, 255, 17, Color{180, 192, 220, 255});
                DrawText(TextFormat("POWER  %02d", (int)(Fighters[index].power * 100)), x + 26, 280, 17, Color{180, 192, 220, 255});
            }
            DrawTextCentered("LEFT / RIGHT select     ENTER confirm     ESC back", 625, 18, Color{134, 153, 190, 255});
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
            DrawTextCentered("LEFT / RIGHT select     ENTER confirm     ESC back", 625, 18, Color{134, 153, 190, 255});
        } else if (screen == Screen::Settings) {
            const Resolution& resolution = Resolutions[resolutionIndex];
            DrawTextCentered("DISPLAY SETTINGS", 80, 42, RAYWHITE);
            DrawText("DISPLAY MODE", 270, 220, 22, Color{180, 192, 220, 255});
            DrawText(GetDisplayModeName(displayMode), 690, 220, 25, settingsFocus == 0 ? Color{64, 196, 255, 255} : RAYWHITE);
            DrawText("RESOLUTION", 270, 320, 22, Color{180, 192, 220, 255});
            DrawText(TextFormat("%d x %d", resolution.width, resolution.height), 690, 320, 25, settingsFocus == 1 ? Color{64, 196, 255, 255} : RAYWHITE);
            DrawTextCentered("UP / DOWN choose     LEFT / RIGHT change", 500, 19, Color{134, 153, 190, 255});
            DrawTextCentered("ENTER or ESC back to menu", 540, 19, Color{134, 153, 190, 255});
        } else if (screen == Screen::Fight || screen == Screen::Pause) {
            DrawText("WAVE  01", 44, 34, 24, Color{134, 153, 190, 255});
            DrawText(TextFormat("SCORE  %05d", (int)score), 1020, 34, 24, RAYWHITE);
            DrawBar({44, 80, 330, 18}, playerHealth, Fighters[selectedFighter].color, Fighters[selectedFighter].name.c_str());
            DrawBar({906, 80, 330, 18}, enemies.empty() ? 0.0f : enemies.front().health, Color{245, 83, 98, 255}, "INCOMING");
            DrawLine(40, (int)FloorY, 1240, (int)FloorY, Color{64, 196, 255, 180});
            DrawFighter(playerPosition, Fighters[selectedFighter].color, true);
            for (const Enemy& enemy : enemies) DrawFighter(enemy.position, Color{156, 87, 211, 255}, false, true);
            DrawArenaGround(Arenas[selectedArena]);
            DrawText("ESC  menu", 44, 670, 17, Color{134, 153, 190, 255});
            if (screen == Screen::Pause) {
                DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{5, 8, 18, 170});
                DrawTextCentered("PAUSED", 170, 64, RAYWHITE);
                const Color continueColor = pauseChoice == 0 ? Color{64, 196, 255, 255} : Color{70, 82, 116, 255};
                const Color menuColor = pauseChoice == 1 ? Color{245, 83, 98, 255} : Color{70, 82, 116, 255};
                DrawRectangle(390, 320, 500, 72, Color{28, 35, 62, 245});
                DrawRectangleLinesEx({390, 320, 500, 72}, 2, continueColor);
                DrawTextCentered("CONTINUE", 342, 27, RAYWHITE);
                DrawRectangle(390, 410, 500, 72, Color{28, 35, 62, 245});
                DrawRectangleLinesEx({390, 410, 500, 72}, 2, menuColor);
                DrawTextCentered("MAIN MENU", 432, 27, RAYWHITE);
                DrawTextCentered("UP / DOWN select     ENTER confirm     ESC continue", 570, 18, Color{180, 192, 220, 255});
            }
        } else if (screen == Screen::GameOver) {
            DrawTextCentered("SYSTEM FAILURE", 190, 62, Color{245, 83, 98, 255});
            DrawTextCentered(TextFormat("FINAL SCORE  %05d", (int)score), 300, 28, RAYWHITE);
            DrawTextCentered("ENTER  choose another fighter", 425, 21, Color{134, 153, 190, 255});
            DrawTextCentered("ESC  main menu", 465, 21, Color{134, 153, 190, 255});
        } else if (screen == Screen::ExitConfirm) {
            DrawTextCentered("EXIT GAME?", 230, 58, Color{245, 83, 98, 255});
            DrawTextCentered("Press ENTER to exit", 350, 25, RAYWHITE);
            DrawTextCentered("Press ESC to return to menu", 405, 21, Color{134, 153, 190, 255});
        }

        EndMode2D();
        EndDrawing();
    }

    CloseAudioDevice();
    UnloadArenaTextures();
    CloseWindow();
    return 0;
}
