#include <raylib.h>
#include <raytiles.h>

int main() {
  const int WinWidth = 640;
  const int WinHeight = 640;
  InitWindow(WinWidth, WinHeight, "Basic Tilemap Example");
  SetTargetFPS(60);

  TileMap t = LoadTileMap("Resources/Basic.json");

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(LIME);
    DrawTileMap(t, 4);
    EndDrawing();
  }

  UnloadTileMap(t);
  CloseWindow();
}
