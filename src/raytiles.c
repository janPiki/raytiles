#include "raytiles.h"
#include "cJSON.h"
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Loading & Unloading functions

char *ReadFile(char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    printf("Failed to open file: %s\n", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long lenght = ftell(file);
  rewind(file);

  char *data = (char *)malloc(lenght + 1);
  if (!data) {
    fclose(file);
    return NULL;
  }

  if (fread(data, 1, lenght, file) != lenght) {
    free(data);
    fclose(file);
    return NULL;
  }
  data[lenght] = '\0';
  fclose(file);

  return data;
}

Object LoadObject(cJSON *object) {
  Object obj = {0};
  cJSON *type = cJSON_GetObjectItem(object, "type");

  char *typestr = type ? type->valuestring : "none";
  obj.type = strdup(typestr);
  if (!obj.type) {
    return (Object){0};
  }

  int x = cJSON_GetObjectItem(object, "x")->valueint;
  int y = cJSON_GetObjectItem(object, "y")->valueint;

  obj.position = (Vector2i){x, y};

  int width = cJSON_GetObjectItem(object, "width")->valueint;
  int height = cJSON_GetObjectItem(object, "height")->valueint;

  obj.size = (Vector2i){width, height};

  return obj;
}

Layer LoadTileLayer(cJSON *layer, Layer l) {
  int width = cJSON_GetObjectItem(layer, "width")->valueint;
  int height = cJSON_GetObjectItem(layer, "height")->valueint;

  l.LayerData.tileLayer.size = (Vector2i){width, height};

  cJSON *data = cJSON_GetObjectItem(layer, "data");

  l.LayerData.tileLayer.data = malloc(height * sizeof(int *));

  for (int i = 0; i < height; i++) {
    l.LayerData.tileLayer.data[i] = malloc(width * sizeof(int));
    for (int j = 0; j < width; j++) {
      l.LayerData.tileLayer.data[i][j] =
          cJSON_GetArrayItem(data, i * width + j)->valueint;
    }
  }
  return l;
}

void UnloadObject(Object obj) {
  if (obj.type) {
    free(obj.type);
  }
}

Layer LoadObjectLayer(cJSON *layer, Layer l) {
  cJSON *objectsArray = cJSON_GetObjectItem(layer, "objects");
  int objectsArraylength = cJSON_GetArraySize(objectsArray);
  l.LayerData.objLayer.objectCount = objectsArraylength;

  l.LayerData.objLayer.objects = malloc(sizeof(Object) * objectsArraylength);

  for (int i = 0; i < objectsArraylength; i++) {
    l.LayerData.objLayer.objects[i] =
        LoadObject(cJSON_GetArrayItem(objectsArray, i));
  }
  return l;
}

Layer LoadImageLayer(cJSON *layer, Layer l) {
  int x = cJSON_GetObjectItem(layer, "x")->valueint;
  int y = cJSON_GetObjectItem(layer, "y")->valueint;

  l.LayerData.imageLayer.position = (Vector2i){x, y};

  char *imageFilename = cJSON_GetObjectItem(layer, "image")->valuestring;
  l.LayerData.imageLayer.image = LoadTexture(imageFilename);

  return l;
}

Layer LoadLayer(cJSON *layer);

Layer LoadLayerGroup(cJSON *layer, Layer l) {
  cJSON *layerInGroup = cJSON_GetObjectItem(layer, "layers");

  int layerCount = cJSON_GetArraySize(layerInGroup);
  l.LayerData.group.layerCount = layerCount;

  l.LayerData.group.layers = malloc(sizeof(Layer) * layerCount);

  for (int i = 0; i < layerCount; i++) {
    l.LayerData.group.layers[i] =
        LoadLayer(cJSON_GetArrayItem(layerInGroup, i));
  }
  return l;
}

Layer LoadLayer(cJSON *layer) {
  Layer l = {0};

  char *typestr = cJSON_GetObjectItem(layer, "type")->valuestring;
  LayerType layerType;
  if (strcmp(typestr, "tilelayer") == 0) {
    layerType = TILE_LAYER;
    l = LoadTileLayer(layer, l);
  } else if (strcmp(typestr, "objectgroup") == 0) {
    layerType = OBJECT_GROUP;
    l = LoadObjectLayer(layer, l);
  } else if (strcmp(typestr, "imagelayer") == 0) {
    layerType = IMAGE_LAYER;
    l = LoadImageLayer(layer, l);
  } else if (strcmp(typestr, "group") == 0) {
    layerType = GROUP;
    l = LoadLayerGroup(layer, l);
  } else {
    printf("Layer type not recognised");
    return l;
  }

  l.type = layerType;
  return l;
}

void UnloadLayer(Layer layer) {
  if (layer.type == TILE_LAYER) {
    for (int i = 0; i < layer.LayerData.tileLayer.size.y; i++) {
      free(layer.LayerData.tileLayer.data[i]);
    }
    free(layer.LayerData.tileLayer.data);
  } else if (layer.type == OBJECT_GROUP) {
    for (int i = 0; i < layer.LayerData.objLayer.objectCount; i++) {
      UnloadObject(layer.LayerData.objLayer.objects[i]);
    }
    free(layer.LayerData.objLayer.objects);
  } else if (layer.type == IMAGE_LAYER) {
    UnloadTexture(layer.LayerData.imageLayer.image);
  } else if (layer.type == GROUP) {
    for (int i = 0; i < layer.LayerData.group.layerCount; i++) {
      UnloadLayer(layer.LayerData.group.layers[i]);
    }
    free(layer.LayerData.group.layers);
  }
}

char *GetDirectory(char *filepath) {
  char *lastSlash = strrchr(filepath, '/');
  if (!lastSlash)
    return strdup("./");

  size_t len = lastSlash - filepath + 1;
  char *dir = malloc(len + 1);
  strncpy(dir, filepath, len);
  dir[len] = '\0';
  return dir;
}

TileSet LoadTileSet(cJSON *data, char *basePath);
char *ResolveRalativePath(char *base, char *relative);

TileMap LoadTileMap(char *filename) {
  char *filedata = ReadFile(filename);
  cJSON *data = cJSON_Parse(filedata);

  if (!data) {
    printf("Failed to parse file");
    free(filedata);
    return (TileMap){0};
  }

  TileMap t = {0};
  t.size.x = cJSON_GetObjectItem(data, "width")->valueint;
  t.size.y = cJSON_GetObjectItem(data, "height")->valueint;

  t.position = (Vector2i){0};

  cJSON *layers = cJSON_GetObjectItem(data, "layers");
  int layerNum = cJSON_GetArraySize(layers);

  t.layerCount = layerNum;
  t.layers = malloc(sizeof(Layer) * layerNum);

  t.tileSize.x = cJSON_GetObjectItem(data, "tilewidth")->valueint;
  t.tileSize.y = cJSON_GetObjectItem(data, "tileheight")->valueint;

  for (int i = 0; i < layerNum; i++) {
    t.layers[i] = LoadLayer(cJSON_GetArrayItem(layers, i));
  }

  cJSON *tilesetsArray = cJSON_GetObjectItem(data, "tilesets");
  cJSON *tilesetStub = cJSON_GetArrayItem(tilesetsArray, 0);

  cJSON *sourceItem = cJSON_GetObjectItem(tilesetStub, "source");

  if (!cJSON_IsString(sourceItem)) {
    char *source = sourceItem->valuestring;
    char *resolvedPath = ResolveRalativePath(filename, source);
    char *tileSetFileData = ReadFile(resolvedPath);
    free(resolvedPath);

    if (!tileSetFileData) {
      printf("Error: Failed to read tileset: %s", source);
      return (TileMap){0};
    }

    cJSON *tilesetData = cJSON_Parse(tileSetFileData);
    free(tileSetFileData);

    if (!tilesetData) {
      printf("Error: Failed to parse file: %s\n", source);
      return (TileMap){0};
    }

    char *tilesetPath = GetDirectory(resolvedPath);
    t.tileSet = LoadTileSet(tilesetData, tilesetPath);
    free(tilesetPath);
    cJSON_Delete(tilesetData);
  } else {
    char *tilemapDir = GetDirectory(filename);
    t.tileSet = LoadTileSet(tilesetStub, tilemapDir);
    free(tilemapDir);
  }

  cJSON_Delete(data);
  free(filedata);
  return t;
}

void UnloadTileMap(TileMap t) {
  if (!t.layers)
    return;
  for (int i = 0; i < t.layerCount; i++) {
    UnloadLayer(t.layers[i]);
  }
  UnloadTileSet(t.tileSet);
  free(t.layers);
}

Tile LoadTile(cJSON *data) {
  Tile t = {0};

  t.id = cJSON_GetObjectItem(data, "id")->valueint;

  cJSON *propertyArray = cJSON_GetObjectItem(data, "properties");
  if (cJSON_IsArray(propertyArray)) {
    t.propertiesCount = cJSON_GetArraySize(propertyArray);
    t.properties = malloc(sizeof(TileProperties) * t.propertiesCount);
    for (int i = 0; i < t.propertiesCount; i++) {
      cJSON *thisProperty = cJSON_GetArrayItem(propertyArray, i);
      t.properties[i].key =
          strdup(cJSON_GetObjectItem(thisProperty, "name")->valuestring);

      char *type = cJSON_GetObjectItem(thisProperty, "type")->valuestring;
      if (strcmp(type, "int") == 0) {
        t.properties[i].type = INT;
        t.properties[i].Value.i =
            cJSON_GetObjectItem(thisProperty, "value")->valueint;

      } else if (strcmp(type, "float") == 0) {
        t.properties[i].type = FLOAT;
        t.properties[i].Value.f =
            cJSON_GetObjectItem(thisProperty, "value")->valuedouble;

      } else if (strcmp(type, "bool") == 0) {
        t.properties[i].type = BOOL;
        t.properties[i].Value.b =
            cJSON_IsTrue(cJSON_GetObjectItem(thisProperty, "value"));

      } else if (strcmp(type, "string") == 0) {
        t.properties[i].type = STRING;
        t.properties[i].Value.s =
            strdup(cJSON_GetObjectItem(thisProperty, "value")->valuestring);
      }
    }
  }

  return t;
}

void UnloadTile(Tile t) {
  for (int i = 0; i < t.propertiesCount; i++) {
    if (t.properties[i].type == STRING && t.properties[i].Value.s) {
      free(t.properties[i].Value.s);
    }
    free(t.properties[i].key);
  }
  free(t.properties);
}

char *ResolveRalativePath(char *basePath, char *relativePath) {
  char *lastSlash = strrchr(basePath, '/');
  if (!lastSlash)
    return strdup(relativePath);
  size_t baseLen = lastSlash - basePath + 1;

  char *fullPath = malloc(baseLen + strlen(relativePath) + 1);
  strncpy(fullPath, basePath, baseLen);
  strcpy(fullPath + baseLen, relativePath);
  return fullPath;
}

TileSet LoadTileSet(cJSON *data, char *dir) {
  cJSON *sourceItem = cJSON_GetObjectItem(data, "source");
  if (cJSON_IsString(sourceItem)) {
    char *source = sourceItem->valuestring;

    if (!dir) {
      printf("Error: Tilemap Filepath not set");
      return (TileSet){0};
    }
    char *sourcePath = sourceItem->valuestring;
    char *resolvedPath = ResolveRalativePath(dir, sourcePath);

    char *tileSetFiledata = ReadFile(resolvedPath);

    if (!tileSetFiledata) {
      printf("Error: Failed to read tileset: %s\n", sourcePath);
      return (TileSet){0};
    }

    cJSON *tileSetData = cJSON_Parse(tileSetFiledata);
    free(tileSetFiledata);

    if (!tileSetData) {
      printf("Error: Failed to parse tileset");
      return (TileSet){0};
    }

    char *tilesetDir = GetDirectory(resolvedPath);
    free(resolvedPath);

    TileSet ts = LoadTileSet(tileSetData, tilesetDir);
    free(tilesetDir);
    cJSON_Delete(tileSetData);
    return ts;
  }

  TileSet s = {0};
  s.size.x = cJSON_GetObjectItem(data, "columns")->valueint;
  s.TileCountWithNoProp = cJSON_GetObjectItem(data, "tilecount")->valueint;
  s.size.y = (s.TileCountWithNoProp + s.size.x - 1) / s.size.x;

  char *imageRelativePath = cJSON_GetObjectItem(data, "image")->valuestring;
  char *imageFullPath = ResolveRalativePath(dir, imageRelativePath);
  s.image = LoadTexture(imageFullPath);
  free(imageFullPath);

  cJSON *tiles = cJSON_GetObjectItem(data, "tiles");
  if (cJSON_IsArray(tiles)) {
    s.TileCount = cJSON_GetArraySize(tiles);
    s.tiles = malloc(sizeof(Tile) * s.TileCount);
    for (int i = 0; i < cJSON_GetArraySize(tiles); i++) {
      s.tiles[i] = LoadTile(cJSON_GetArrayItem(tiles, i));
    }
  }

  return s;
}

TileSet LoadTileSetFromFile(char *filename) {
  char *filedata = ReadFile(filename);
  cJSON *data = cJSON_Parse(filedata);
  if (!data) {
    printf("Failed to parse file");
    return (TileSet){0};
  }

  char *dir = GetDirectory(filename);
  TileSet s = LoadTileSet(data, dir);
  cJSON_Delete(data);
  free(filedata);
  return s;
}

void UnloadTileSet(TileSet tileSet) {
  UnloadTexture(tileSet.image);

  if (tileSet.tiles) {
    for (int i = 0; i < tileSet.TileCount; i++) {
      UnloadTile(tileSet.tiles[i]);
    }
    free(tileSet.tiles);
  }
}
// Loading & unloading functions

// Drawing functions

Rectangle GetSourceRec(int id, Vector2i tileSize, int imageWidth, TileSet s) {
  int columns = imageWidth / tileSize.x;

  int x = (id % columns) * tileSize.x;
  int y = (id / columns) * tileSize.y;

  return (Rectangle){x, y, tileSize.x, tileSize.y};
}

void DrawTileMap(TileMap t, int scale) {
  for (int i = 0; i < t.layerCount; i++) {
    DrawLayer(t, t.layers[i], scale);
  }
}

void DrawLayer(TileMap t, Layer l, int scale) {
  switch (l.type) {
  case TILE_LAYER:
    for (int x = 0; x < l.LayerData.tileLayer.size.x; x++) {
      for (int y = 0; y < l.LayerData.tileLayer.size.y; y++) {
        DrawTile(t, l, (Vector2i){x, y}, scale);
      }
    }
    break;
  case OBJECT_GROUP:
    break;
  case IMAGE_LAYER:
    DrawTexture(l.LayerData.imageLayer.image, l.LayerData.imageLayer.position.x,
                l.LayerData.imageLayer.position.y, WHITE);
    break;
  case GROUP:
    for (int i = 0; i < l.LayerData.group.layerCount; i++) {
      DrawLayer(t, l.LayerData.group.layers[i], scale);
    }
    break;
  }
}

void DrawTile(TileMap t, Layer l, Vector2i pos, int scale) {
  bool outOfBounds = pos.x >= l.LayerData.tileLayer.size.x ||
                     pos.y >= l.LayerData.tileLayer.size.y || pos.x < 0 ||
                     pos.y < 0;

  if (outOfBounds)
    return;

  int id = l.LayerData.tileLayer.data[pos.y][pos.x];
  if (id == 0) {
    return;
  }

  Rectangle src =
      GetSourceRec(id - 1, t.tileSize, t.tileSet.image.width, t.tileSet);

  float posWorldX = t.tileSize.x * scale * pos.x;
  float posWorldY = t.tileSize.y * scale * pos.y;

  DrawTexturePro(t.tileSet.image, src,
                 (Rectangle){posWorldX, posWorldY, t.tileSize.x * scale,
                             t.tileSize.y * scale},
                 (Vector2){0}, 0, WHITE);
}

// Drawing functions

// Type convertion

Vector2i WorldToGrid(Vector2 worldPos, Vector2i tileSize) {
  int flooredX = (int)worldPos.x / tileSize.x;
  int flooredY = (int)worldPos.y / tileSize.y;
  return (Vector2i){flooredX, flooredY};
}

Vector2 GridToWorld(Vector2i gridPos, Vector2i tileSize) {
  float worldX = gridPos.x * tileSize.x;
  float worldY = gridPos.y * tileSize.y;
  return (Vector2){worldX, worldY};
}

// Type convertion

// Properties & types

Tile GetTileFromId(int id, TileSet s) {
  if (id == 0) {
    return (Tile){0};
  }

  id -= 1;

  for (int i = 0; i < s.TileCount; i++) {
    if (s.tiles[i].id == id) {
      return s.tiles[i];
    }
  }
  return (Tile){0};
}

int GetTilePropertyInt(TileMap t, Layer l, Vector2i pos, char *key) {
  if (l.LayerData.tileLayer.data[pos.y][pos.x] == 0) {
    return 0;
  }
  Tile tile =
      GetTileFromId(l.LayerData.tileLayer.data[pos.y][pos.x], t.tileSet);
  for (int i = 0; i < tile.propertiesCount; i++) {
    if (strcmp(tile.properties[i].key, key) == 0) {
      return tile.properties[i].Value.i;
    }
  }
  return 0;
}

float GetTilePropertyFloat(TileMap t, Layer l, Vector2i pos, char *key) {
  if (l.LayerData.tileLayer.data[pos.y][pos.x] == 0) {
    return 0;
  }
  Tile tile =
      GetTileFromId(l.LayerData.tileLayer.data[pos.y][pos.x], t.tileSet);
  for (int i = 0; i < tile.propertiesCount; i++) {
    if (strcmp(tile.properties[i].key, key) == 0) {
      return tile.properties[i].Value.f;
    }
  }
  return 0;
}

bool GetTilePropertyBool(TileMap t, Layer l, Vector2i pos, char *key) {
  if (l.LayerData.tileLayer.data[pos.y][pos.x] == 0) {
    return 0;
  }
  Tile tile =
      GetTileFromId(l.LayerData.tileLayer.data[pos.y][pos.x], t.tileSet);
  for (int i = 0; i < tile.propertiesCount; i++) {
    if (strcmp(tile.properties[i].key, key) == 0) {
      return tile.properties[i].Value.b;
    }
  }
  return 0;
}

char *GetTilePropertyString(TileMap t, Layer l, Vector2i pos, char *key) {
  if (l.LayerData.tileLayer.data[pos.y][pos.x] == 0) {
    return 0;
  }
  Tile tile =
      GetTileFromId(l.LayerData.tileLayer.data[pos.y][pos.x], t.tileSet);
  for (int i = 0; i < tile.propertiesCount; i++) {
    if (strcmp(tile.properties[i].key, key) == 0) {
      return tile.properties[i].Value.s;
    }
  }
  return 0;
}
// Properties & types
