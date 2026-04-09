#pragma once

typedef struct {
  int locked;
} umutex_t;

void mutex_init(umutex_t *m);
void mutex_lock(umutex_t *m);
void mutex_unlock(umutex_t *m);
