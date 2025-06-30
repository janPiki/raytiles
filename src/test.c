#include "raytiles.h"
#include <raylib.h>

int main() {
  InitWindow(600, 600, "Its tilemaps!");
  SetTargetFPS(60);

  TileMap t = LoadTileMap("untitled.json");

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(SKYBLUE);
    DrawTileMap(t);
    EndDrawing();
  }

  UnloadTileMap(t);
  CloseWindow();
}
