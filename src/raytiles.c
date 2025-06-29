#include "raytiles.h"
#include "../cJSON.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  return data;
}

Object LoadObject(cJSON *object) {
  Object obj;
  obj.type = cJSON_GetObjectItem(object, "type")->valuestring;

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

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      l.LayerData.tileLayer.data[j][i] =
          cJSON_GetArrayItem(data, i * width + j)->valueint;
    }
  }
  return l;
}

Layer LoadObjectLayer(cJSON *layer, Layer l) {
  cJSON *objectsArray = cJSON_GetObjectItem(layer, "objects");
  int objectsArraylength = cJSON_GetArraySize(objectsArray);
  l.LayerData.objLayer.objectCount = objectsArraylength;

  for (int i = 0; i < objectsArraylength; i++) {
    l.LayerData.objLayer.objects[i] = LoadObject(cJSON_GetArrayItem(layer, i));
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

  for (int i = 0; i < layerCount; i++) {
    l.LayerData.group.layers[i] =
        LoadLayer(cJSON_GetArrayItem(layerInGroup, i));
  }
  return l;
}

Layer LoadLayer(cJSON *layer) {
  Layer l;

  char *typestr = cJSON_GetObjectItem(layer, "type")->valuestring;
  LayerType layerType;
  if (strcmp(typestr, "tilelayer") == 0) {
    layerType = TILE_LAYER;
    l = LoadTileLayer(layer, l);
  } else if (strcmp(typestr, "objectgroup")) {
    // Load Object Layer //
    layerType = OBJECT_GROUP;
    l = LoadObjectLayer(layer, l);
  } else if (strcmp(typestr, "imagelayer")) {
    // Load Image Layer //
    layerType = IMAGE_LAYER;
    l = LoadImageLayer(layer, l);
  } else if (strcmp(typestr, "group") == 0) {
    // Load Layer Group //
    layerType = GROUP;
    l = LoadLayerGroup(layer, l);
  }

  l.type = layerType;
  return l;
}

void UnloadLayer(Layer layer) {}

TileMap LoadTileMap(char *filename) {
  cJSON *data = cJSON_Parse(ReadFile(filename));

  TileMap t;
  t.size.x = cJSON_GetObjectItem(data, "width")->valueint;
  t.size.y = cJSON_GetObjectItem(data, "height")->valueint;

  cJSON *layers = cJSON_GetObjectItem(data, "layers");
  int layerNum = cJSON_GetArraySize(layers);

  for (int i = 0; i < layerNum; i++) {
    t.layers[i] = LoadLayer(cJSON_GetArrayItem(layers, i));
  }

  return t;
}
