#include "raytiles.h"
#include "../cJSON.h"
#include <alloca.h>
#include <complex.h>
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Loading & Unloading functions

char *ReadFile(char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    printf("Failed to open file");
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

  fread(data, 1, lenght, file);
  data[lenght] = '\0';
  fclose(file);

  if (!data) {
    printf("Failed to read file");
    return "";
  }
  return data;
}

Object LoadObject(cJSON *object) {
  Object obj = {0};
  cJSON *type = cJSON_GetObjectItem(object, "type");
  obj.type = type ? strdup(type->valuestring) : strdup("none");

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

TileSet LoadTileSet(cJSON *data);

TileMap LoadTileMap(char *filename) {
  char *filedata = ReadFile(filename);
  cJSON *data = cJSON_Parse(filedata);

  if (!data) {
    printf("Failed to parse file");
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
  cJSON *tileset = cJSON_GetArrayItem(tilesetsArray, 0);

  cJSON *sourcecJSON = cJSON_GetObjectItem(tileset, "source");

  if (cJSON_IsString(sourcecJSON)) {
    char *source = sourcecJSON->valuestring;
    t.tileSet = LoadTileSetFromFile(
        source, cJSON_GetObjectItem(tileset, "firstgit")->valueint);
  } else {
    t.tileSet = LoadTileSet(tileset);
  }

  cJSON_Delete(data);
  free(filedata);
  return t;
}

void UnloadTileMap(TileMap t) {
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

TileSet LoadTileSet(cJSON *data) {
  TileSet s = {0};
  s.size.x = cJSON_GetObjectItem(data, "columns")->valueint;
  s.TileCount = cJSON_GetObjectItem(data, "tilecount")->valueint;
  s.size.y = (s.TileCount + s.size.x - 1) / s.size.x;

  s.image = LoadTexture(cJSON_GetObjectItem(data, "image")->valuestring);

  s.firstId = cJSON_GetObjectItem(data, "firstgid")->valueint;

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

TileSet LoadTileSetFromFile(char *filename, int firstId) {
  char *filedata = ReadFile(filename);
  cJSON *data = cJSON_Parse(filedata);
  if (!data) {
    printf("Failed to parse file");
    return (TileSet){0};
  }

  TileSet s = LoadTileSet(data);
  s.firstId = firstId;
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

Rectangle GetSourceRec(int id, int firstid, Vector2i tileSize,
                       int tileSetWidth) {
  int localId = id - firstid;
  int columns = tileSetWidth / tileSize.x;

  int x = (localId % columns) * tileSize.x;
  int y = (localId / columns) * tileSize.y;

  return (Rectangle){x, y, tileSize.x, tileSize.y};
}

void DrawTileMap(TileMap t) {
  for (int i = 0; i < t.layerCount; i++) {
    DrawLayer(t, i);
  }
}

void DrawLayer(TileMap t, int layer) {
  switch (t.layers[layer].type) {
  case TILE_LAYER:
    for (int x = 0; x < t.layers[layer].LayerData.tileLayer.size.x; x++) {
      for (int y = 0; y < t.layers[layer].LayerData.tileLayer.size.y; y++) {
        DrawTile(t, t.layers[layer], (Vector2i){x, y});
      }
    }
    break;
  case OBJECT_GROUP:
    break;
  case IMAGE_LAYER:
    DrawTexture(t.layers[layer].LayerData.imageLayer.image,
                t.layers[layer].LayerData.imageLayer.position.x,
                t.layers[layer].LayerData.imageLayer.position.y, WHITE);
    break;
  case GROUP:
    for (int i = 0; i < t.layers[layer].LayerData.group.layerCount; i++) {
      // I'll do this later;
    }
    break;
  }
}

void DrawTile(TileMap t, Layer l, Vector2i pos) {
  bool posTooBig = pos.x >= l.LayerData.tileLayer.size.x ||
                   pos.y >= l.LayerData.tileLayer.size.y;
  if (pos.x < 0 || pos.y < 0 || posTooBig) {
    return;
  }

  int id = l.LayerData.tileLayer.data[pos.y][pos.x];
  if (id == 0) {
    return;
  }

  Rectangle src =
      GetSourceRec(id, t.tileSet.firstId, t.tileSize, t.tileSet.image.width);

  float posWorldX = t.tileSize.x * pos.x;
  float posWorldY = t.tileSize.y * pos.y;

  DrawTextureRec(t.tileSet.image, src, (Vector2){posWorldX, posWorldY}, WHITE);
}

// Drawing functions

// Type convertion

Vector2i WorldToGrid(Vector2 worldPos, Vector2i tileSize) {
  int flooredX = floor(worldPos.x / tileSize.x);
  int flooredY = floor(worldPos.y / tileSize.y);
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
  for (int i = 0; i < s.TileCount; i++) {
    if (s.tiles[i].id == id) {
      return s.tiles[i];
    }
  }
  return (Tile){0};
}

int GetTilePropertyInt(TileMap t, int layer, Vector2i pos, char *key) {
  Tile tile = GetTileFromId(
      t.layers[layer].LayerData.tileLayer.data[pos.y][pos.x], t.tileSet);
  for (int i = 0; i < tile.propertiesCount; i++) {
    if (strcmp(tile.properties[i].key, key) == 0) {
      return tile.properties[i].Value.i;
    }
  }
  return 0;
}

float GetTilePropertyFloat(TileMap t, int layer, Vector2i pos, char *key) {
  Tile tile = GetTileFromId(
      t.layers[layer].LayerData.tileLayer.data[pos.y][pos.x], t.tileSet);
  for (int i = 0; i < tile.propertiesCount; i++) {
    if (strcmp(tile.properties[i].key, key) == 0) {
      return tile.properties[i].Value.f;
    }
  }
  return 0;
}

bool GetTilePropertyBool(TileMap t, int layer, Vector2i pos, char *key) {
  Tile tile = GetTileFromId(
      t.layers[layer].LayerData.tileLayer.data[pos.y][pos.x], t.tileSet);
  for (int i = 0; i < tile.propertiesCount; i++) {
    if (strcmp(tile.properties[i].key, key) == 0) {
      return tile.properties[i].Value.b;
    }
  }
  return 0;
}

char *GetTilePropertyString(TileMap t, int layer, Vector2i pos, char *key) {
  Tile tile = GetTileFromId(
      t.layers[layer].LayerData.tileLayer.data[pos.y][pos.x], t.tileSet);
  for (int i = 0; i < tile.propertiesCount; i++) {
    if (strcmp(tile.properties[i].key, key) == 0) {
      return tile.properties[i].Value.s;
    }
  }
  return 0;
}

// Properties & types
