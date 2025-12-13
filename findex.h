#ifndef FINDEX_H
#define FINDEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define FINDEX_NAME_SIZE (256)  // 256 characters
#define FINDEX_EXT_SIZE (32)    // 32 characters

enum FINDEX_TYPE_e {
  FINDEX_DIR = 0,
  FINDEX_FILE,
  FINDEX_UNKNOWN,
};

typedef enum FINDEX_TYPE_e FINDEX_TYPE;
typedef struct Findex_s Findex;
typedef struct Findex_Array_s Findex_Array;
typedef struct Findex_Queue_s Findex_Queue;

struct Findex_Array_s {
  Findex *items;
  size_t size;
  size_t capacity;
};

struct Findex_Queue_s {
  Findex **items;
  size_t size;
  size_t capacity;
};

struct Findex_s {
  char *full_path;

  char name[FINDEX_NAME_SIZE];
  char ext[FINDEX_EXT_SIZE];

  FINDEX_TYPE type;

  Findex *parent;
  Findex_Array children;
};

int findex_scan(Findex *node, char *path);
int findex_free(Findex *node);
void findex_print(Findex *node, size_t depth);

#endif  // FINDEX_H


// #ifdef FINDEX_IMPLEMENTATION

// Private functions forward declarations
Findex *findex__request_child(Findex_Array *array);
int findex__queue_append(Findex_Queue *queue, Findex *element);


int findex_scan(Findex *node, char *path) {
  if (path == NULL || node == NULL) {
    printf("[ERROR]: "
           "Error in findex_scan: "
           "path or node is null\n");
    return -1;
  }

  if (path[strlen(path) - 1] != '/') {
    printf("[ERROR]: "
           "Error in findex_scan: "
           "The path given is not a directory\n");
    return -1;
  }

  // Root note will be empty
  node->type = FINDEX_DIR;
  node->full_path = strdup(path);
  strncpy(node->name, node->full_path, 255);

  Findex_Queue queue_findex = {0};
  size_t curr_queue_idx = 0;

  Findex *curr_dir = node;

  struct dirent *curr_dirent = NULL;
  DIR *dir = NULL;

  while (curr_dir != NULL) {
    // Cycle directories
    printf("[DEBUG]: Scanning folder %s\n", curr_dir->full_path);
    dir = opendir(curr_dir->full_path);
    if (dir == NULL) {
      printf("[ERROR]: "
             "Error in findex_scan: "
             "Could not open directory\n");
      return -1;
    }

    while ((curr_dirent = readdir(dir)) != NULL) {
      // Cycle single directory
      if (strncmp(curr_dirent->d_name, ".", strlen(curr_dirent->d_name)) == 0 ||
          strncmp(curr_dirent->d_name, "..", strlen(curr_dirent->d_name)) == 0) {
        continue;
      }

      Findex *curr_findex = findex__request_child(&curr_dir->children);
      strncpy(curr_findex->name, curr_dirent->d_name, 255);
      char *ext = strrchr(curr_dirent->d_name, '.');
      if (ext != NULL) {
        strncpy(curr_findex->ext, ext, 31);
      }
      curr_findex->parent = curr_dir;

      if (curr_dirent->d_type == DT_DIR) {
        curr_findex->type = FINDEX_DIR;
        curr_findex->full_path = calloc(2048, sizeof(char));
        snprintf(curr_findex->full_path, 2047, "%s%s/", curr_dir->full_path, curr_findex->name);
        if (findex__queue_append(&queue_findex, curr_findex) < 0) {
          return -1;
        }
      }
      else if (curr_dirent->d_type == DT_REG) {
        curr_findex->type = FINDEX_FILE;
        curr_findex->full_path = curr_dir->full_path;
      }
      else {
        curr_findex->type = FINDEX_UNKNOWN;
        curr_findex->full_path = curr_dir->full_path;
      }
    }

    if (curr_queue_idx >= queue_findex.size) {
      // No more direcories to scan
      break;
    }

    if (closedir(dir) < 0) {
      printf("[ERROR]: "
             "Error in findex_scan: "
             "Could not close directory\n");
      return -1;
    }

    dir = NULL;
    curr_dirent = NULL;
    curr_dir = queue_findex.items[curr_queue_idx];
    curr_queue_idx += 1;
  }

  return 0;
}

int findex_free(Findex *node) {
  if (node == NULL || node->children.items == NULL) {
    return 0;
  }

  for (size_t i = 0; i < node->children.size; i++) {
    findex_free(&node->children.items[i]);
  }

  free(node->children.items);
  node->children.items = NULL;
  node->children.capacity = 0;
  node->children.size = 0;

  return 0;
}

void findex_print(Findex *node, size_t depth) {
  if (node == NULL) {
    return;
  }

  printf("D %*s%s -  %ld\n", 2 * (int)depth, "", node->name, node->children.size);
  for (size_t i = 0; i < node->children.size; i++) {
    Findex *item = &(node->children.items[i]);
    if (item->type == FINDEX_FILE) {
      printf("F %*s%s\n", 2 * (int)(depth + 1), "", item->name);
    }
    else if (item->type == FINDEX_DIR) {
      findex_print(item, depth + 1);
    }
    else {
      printf("U %*s%s\n", 2 * (int)(depth + 1), "", item->name);
    }
  }

  return;
}


// Private functions

Findex *findex__request_child(Findex_Array *array) {
  if (array == NULL) {
    return NULL;
  }

  // Initialize array
  if (array->capacity == 0) {
    array->capacity = 1;
    array->items = calloc(array->capacity, sizeof(Findex));
    if (array->items == NULL) {
      printf("[ERROR]: "
             "Error in findex__request_child: "
             "Could not allocate memory\n");
      return NULL;
    }
  }

  // Expand array
  if (array->size >= array->capacity) {
    Findex *temp = calloc(array->capacity * 2, sizeof(Findex));
    if (temp == NULL) {
      printf("[ERROR]: "
             "Error in findex__request_child: "
             "Could not allocate memory\n");
      return NULL;
    }

    // Copy memory
    memcpy(temp, array->items, array->size * sizeof(Findex));
    free(array->items);
    array->items = temp;
    array->capacity *= 2;
  }

  return &array->items[array->size++];
}

int findex__queue_append(Findex_Queue *queue, Findex *element) {
  if (queue == NULL) {
    return -1;
  }

  // Initialize queue
  if (queue->capacity == 0) {
    queue->capacity = 1;
    queue->items = calloc(queue->capacity, sizeof(Findex *));
    if (queue->items == NULL) {
      printf("[ERROR]: "
             "Error in findex__queue_append: "
             "Could not allocate memory\n");
      return -1;
    }
  }

  // Expand queue
  if (queue->size >= queue->capacity) {
    Findex **temp = calloc(queue->capacity * 2, sizeof(Findex *));
    if (temp == NULL) {
      printf("[ERROR]: "
             "Error in findex__request_child: "
             "Could not allocate memory\n");
      return -1;
    }

    // Copy memory
    memcpy(temp, queue->items, queue->size * sizeof(Findex *));
    free(queue->items);
    queue->items = temp;
    queue->capacity *= 2;
  }

  *(queue->items + queue->size) = element;
  queue->size++;

  return 0;
}


// #endif  // FINDEX_IMPLEMENTATION