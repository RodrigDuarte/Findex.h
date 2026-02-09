#ifndef FINDEX_H
#define FINDEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define FINDEX_NAME_SIZE (256)  // 256 characters
#define FINDEX_EXT_SIZE (32)    // 32 characters

enum FINDEX_TYPE_e {
  FINDEX_DIR     = 0,
  FINDEX_FILE    = 1,
  FINDEX_UNKNOWN = 2,
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
  char full_path[PATH_MAX];

  char name[FINDEX_NAME_SIZE];
  char ext[FINDEX_EXT_SIZE];

  FINDEX_TYPE type;

  Findex *parent;
  Findex_Array children;
};

int findex_scan(Findex *node, char *path);
int findex_free(Findex *node);
void findex_print(Findex *node, size_t depth);
char *findex_get_file_location(Findex *node, size_t *file_location_len);

#endif  // FINDEX_H


#ifdef FINDEX_IMPLEMENTATION

// Private variable declarations
char findex__buffer[PATH_MAX] = {0};

// Private function declarations
Findex *findex__request_child(Findex_Array *array);
int findex__queue_append(Findex_Queue *queue, Findex *element);


int findex_scan(Findex *node, char *path) {
  if (node == NULL || path == NULL) {
    printf("[ERROR]: "
           "Error in findex_scan: "
           "path or node is null\n"
    );
    return -1;
  }

  if (path[strlen(path) - 1] != '/') {
    printf("[ERROR]: "
           "Error in findex_scan: "
           "The path given is not a directory\n"
    );
    return -1;
  }

  // Root node will be empty
  node->type = FINDEX_DIR;
  strncpy(node->full_path, path, PATH_MAX - 1);
  strncpy(node->name, node->full_path, FINDEX_NAME_SIZE - 1);

  Findex_Queue queue_findex = {0};
  size_t curr_queue_idx = 0;

  Findex *curr_dir = node;

  struct dirent *dirent = NULL;
  DIR *dir = NULL;

  while (curr_dir != NULL) {

    // Open dir to work with
    dir = opendir(curr_dir->full_path);
    if (dir == NULL) {
      printf("[ERROR]: "
             "Error in findex_scan: "
             "Could not open directory\n");
      return -1;
    }

    // Read entire directory
    while ((dirent = readdir(dir)) != NULL) {
      // Skip current and previous folders
      size_t name_len = strlen(dirent->d_name);
      if (strncmp(dirent->d_name, "." , name_len) == 0 ||
          strncmp(dirent->d_name, "..", name_len) == 0
      ) {
        continue;
      }

      Findex *new_findex = findex__request_child(&curr_dir->children);
      strncpy(new_findex->name, dirent->d_name, FINDEX_NAME_SIZE - 1);
      char *ext = strrchr(dirent->d_name, '.');
      if (ext != NULL) {
        strncpy(new_findex->ext, ext, FINDEX_EXT_SIZE - 1);
      }
      new_findex->parent = curr_dir;

      switch (dirent->d_type) {
        case DT_DIR: {
          new_findex->type = FINDEX_DIR;
          snprintf(new_findex->full_path, PATH_MAX - 1, "%s%s/", curr_dir->full_path, new_findex->name);
        } break;
        case DT_REG: {
          new_findex->type = FINDEX_FILE;
          strncpy(new_findex->full_path, curr_dir->full_path, PATH_MAX);
        } break;
        default: {
          new_findex->type = FINDEX_UNKNOWN;
          strncpy(new_findex->full_path, curr_dir->full_path, PATH_MAX);
        } break;
      }
    }

    // Add folders to analyze
    for (size_t i = 0; i < curr_dir->children.size; i++) {
      Findex *ptr = &(curr_dir->children.items[i]);
      if (ptr->type == FINDEX_DIR) {
        findex__queue_append(&queue_findex, ptr);
      }
    }

    if (closedir(dir) < 0) {
      printf("[ERROR]: "
             "Error in findex_scan: "
             "Could not close directory\n");
      return -1;
    }

    // Finished processing queue
    if (curr_queue_idx >= queue_findex.size) {
      free(queue_findex.items);
      break;
    }

    dir = NULL;
    dirent = NULL;
    curr_dir = queue_findex.items[curr_queue_idx];
    curr_queue_idx++;
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

char *findex_get_file_location(Findex *node, size_t *file_location_len) {
  if (node->type != FINDEX_FILE) {
    *file_location_len = 0;
    return NULL;
  }

  memset(findex__buffer, 0, sizeof(findex__buffer));
  size_t full_path_len = strlen(node->full_path);
  size_t name_len      = strlen(node->name);

  if (full_path_len + name_len >= sizeof(findex__buffer)) {
    *file_location_len = 0;
    return NULL;
  }

  memcpy(findex__buffer, node->full_path, full_path_len);
  memcpy(findex__buffer + full_path_len, node->name, name_len);

  *file_location_len = full_path_len + name_len;

  return findex__buffer;
}

// Private functions

Findex *findex__request_child(Findex_Array *array) {
  if (array == NULL) {
    return NULL;
  }

  // Initialize array
  if (array->capacity == 0) {
    array->capacity = 1;
    array->items = malloc(sizeof(Findex) * array->capacity);
    if (array->items == NULL) {
      printf("[ERROR]: "
             "Error in findex__request_child: "
             "Could not allocate memory for items\n");
      return NULL;
    }
    memset(array->items, 0, sizeof(Findex) * array->capacity);
  }

  // Expand array if needed
  if (array->size >= array->capacity) {
    Findex *temp = malloc(sizeof(Findex) * (array->capacity * 2));
    if (temp == NULL) {
      printf("[ERROR]: "
             "Error in findex__request_child: "
             "Could not allocate more memory for items\n");
      return NULL;
    }
    memset(temp, 0, sizeof(Findex) * (array->capacity * 2));

    // Copy existing Findexes
    memcpy(temp, array->items, array->size * sizeof(Findex));
    free(array->items);
    array->items = temp;
    array->capacity *= 2;
  }

  return &array->items[array->size++];
}

int findex__queue_append(Findex_Queue *queue, Findex *element) {
  if (queue == NULL || element == NULL) {
    return -1;
  }

  // Initialize queue
  if (queue->capacity == 0) {
    queue->capacity = 1;
    queue->items = malloc(sizeof(Findex *) * queue->capacity);
    if (queue->items == NULL) {
      printf("[ERROR]: "
             "Error in findex__queue_append: "
             "Could not allocate memory\n");
      return -1;
    }
    memset(queue->items, 0, sizeof(Findex *) * queue->capacity);
  }

  // Expand queue if needed
  if (queue->size >= queue->capacity) {
    Findex **temp = malloc(sizeof(Findex *) * (queue->capacity * 2));
    if (temp == NULL) {
      printf("[ERROR]: "
             "Error in findex__request_child: "
             "Could not allocate memory\n");
      return -1;
    }
    memset(temp, 0, sizeof(Findex *) * (queue->capacity * 2));

    // Copy existing Findex pointers
    memcpy(temp, queue->items, queue->size * sizeof(Findex *));
    free(queue->items);
    queue->items = temp;
    queue->capacity *= 2;
  }

  *(queue->items + queue->size) = element;
  queue->size++;

  return 0;
}


#endif  // FINDEX_IMPLEMENTATION