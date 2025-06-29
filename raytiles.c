#include "raytiles.h"
#include "cJSON.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ReadFile(char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    printf("Failed to open file or something");
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
  return data;
}

Layer LoadLayer(cJSON *layer) {
  int width = cJSON_GetObjectItem(layer, "width")->valueint;
  int height = cJSON_GetObjectItem(layer, "height")->valueint;
  Layer l = CreateTileLayer((Vector2i){width, height});

  char *typestr = cJSON_GetObjectItem(layer, "type")->valuestring;
  LayerType layerType;
  if (strcmp(typestr, "tilelayer") == 0) {
    layerType = TILE_LAYER;

  } else if (strcmp(typestr, "objectgroup")) {
    layerType = OBJECT_GROUP;
  } else if (strcmp(typestr, "imagelayer")) {
    layerType = IMAGE_LAYER;
  } else if (strcmp(typestr, "group")) {
    layerType = GROUP;
  }

  return l;
}

void UnloadLayer(Layer layer) {}

TileMap LoadTileMap(char *filename) {
  cJSON *data = cJSON_Parse(ReadFile(filename));

  TileMap t = CreateTileMap();
  t.size.x = cJSON_GetObjectItem(data, "width")->valueint;
  t.size.y = cJSON_GetObjectItem(data, "height")->valueint;

  cJSON *layers = cJSON_GetObjectItem(data, "layers");
  int layerNum = cJSON_GetArraySize(layers);

  for (int i = 0; i < layerNum; i++) {
    t.layers[i] = LoadLayer(cJSON_GetArrayItem(layers, i));
  }

  return t;
}
