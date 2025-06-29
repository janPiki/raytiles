#include "raytiles.h"
#include "../cJSON.h"
#include <complex.h>
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
  Object obj = {0};
  obj.type = strdup(cJSON_GetObjectItem(object, "type")->valuestring);

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

Layer LoadObjectLayer(cJSON *layer, Layer l) {
  cJSON *objectsArray = cJSON_GetObjectItem(layer, "objects");
  int objectsArraylength = cJSON_GetArraySize(objectsArray);
  l.LayerData.objLayer.objectCount = objectsArraylength;

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
      free(layer.LayerData.objLayer.objects[i].type);
    }
  } else if (layer.type == IMAGE_LAYER) {
    UnloadTexture(layer.LayerData.imageLayer.image);
  } else if (layer.type == GROUP) {
    for (int i = 0; i < layer.LayerData.group.layerCount; i++) {
      UnloadLayer(layer.LayerData.group.layers[i]);
    }
  }
}

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

  cJSON *layers = cJSON_GetObjectItem(data, "layers");
  int layerNum = cJSON_GetArraySize(layers);

  t.layerCount = layerNum;
  t.layers = malloc(sizeof(Layer) * layerNum);

  for (int i = 0; i < layerNum; i++) {
    t.layers[i] = LoadLayer(cJSON_GetArrayItem(layers, i));
  }

  cJSON_Delete(data);
  free(filedata);
  return t;
}

void UnloadTileMap(TileMap t) {
  for (int i = 0; i < t.layerCount; i++) {
    UnloadLayer(t.layers[i]);
  }
  free(t.layers);
}
