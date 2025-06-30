#ifndef RAYTILE
#define RAYTILE

#include <raylib.h>
#include <stdbool.h>

// Structs and enums
typedef enum { INT, FLOAT, BOOL, STRING } PropertyType;
typedef enum { TILE_LAYER, OBJECT_GROUP, IMAGE_LAYER, GROUP } LayerType;

typedef struct {
  int x, y;
} Vector2i;

typedef struct {
  char *key;
  PropertyType type;
  union {
    int i;
    float f;
    bool b;
    char *s;
  } Value;
} TileProperties;

typedef struct {
  int id;
  TileProperties *properties;
  int propertiesCount;
} Tile;

typedef struct {
  Vector2i size;
  int TileCount;
  Texture2D image;
  Tile *tiles;
  int firstId;
} TileSet;

// Layer types and stuff
struct Layer;

typedef struct {
  Vector2i size;
  int **data; // Double pointer. Spooky!
} TileLayerData;

typedef struct {
  char *type; // Defined by user
  Vector2i position;
  Vector2i size;
} Object;

typedef struct {
  Object *objects;
  int objectCount;
} ObjectLayer;

typedef struct {
  Texture2D image;
  Vector2i position;
} ImageLayer;

typedef struct {
  struct Layer *layers;
  int layerCount;
} Group;

typedef struct Layer {
  char *name;
  LayerType type;
  union {
    TileLayerData tileLayer;
    ObjectLayer objLayer;
    ImageLayer imageLayer;
    Group group;
  } LayerData;
} Layer;

typedef struct {
  Vector2i size;
  Vector2i tileSize;
  Vector2i position;
  TileSet tileSet;
  Layer *layers;
  int layerCount;
} TileMap;

// Function declaration
// Loading & unloading
TileMap LoadTileMap(char *filename);
void UnloadTileMap(TileMap tileMap);
TileSet LoadTileSetFromFile(char *filename, int firstId);
void UnloadTileSet(TileSet tileSet);

// Drawing and stuff
void DrawTileMap(TileMap tileMap);    // Draws all the layers of a TileMap
void DrawLayer(TileMap t, int layer); // Draw one TileMap Layer
void DrawTile(TileMap t, Layer layer, Vector2i position); // Draw a single Tile

// Type convertion
Vector2i WorldToGrid(Vector2 worldPos,
                     Vector2i tileSize); // Convert Vector2 position to grid pos
Vector2 GridToWorld(Vector2i gridPos,
                    Vector2i tileSize); // Convert grid position to Vector2

// Properties & types
int GetTilePropertyInt(TileMap tileMap, int layer, Vector2i pos, char *key);
float GetTilePropertyFloat(TileMap tileMap, int layer, Vector2i pos, char *key);
bool GetTilePropertyBool(TileMap tileMap, int layer, Vector2i pos, char *key);
char *GetTilePropertyString(TileMap tileMap, int layer, Vector2i pos,
                            char *key);

#endif
