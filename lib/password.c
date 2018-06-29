#include "password.h"
#include "base64.h"
#include "log.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* private function */

static void swap(byte *a, byte *b) {
  byte t = *a;
  *a = *b;
  *b = t;
}

/* private function end*/

// represent password as string encode in base64
char *password_string(Password *pass, size_t *outsize) {
  return b64_encode(pass->x, PASS_LENGTH, outsize);
}

// generate password by string encode in base64
Password *gen_password_by_string(char *string, size_t len) {
  Password *pass = calloc(1, sizeof(Password));
  size_t decsize;
  unsigned char *decoded = b64_decode_ex(string, len, &decsize);
  log_d("password size: %ld", decsize);
  check(decsize == PASS_LENGTH, "can't generate password!");
  // strncpy(pass->x, decoded, PASS_LENGTH);
  // copy data
  // wtf: don't use strncpy...
  for (int i = 0; i < PASS_LENGTH; i++) {
    pass->x[i] = decoded[i];
  }
  free(decoded);

  return pass;

error:
  free(pass);
  free(decoded);
  return NULL;
}

Password *rand_password() {
  Password *password = calloc(1, sizeof(Password));
  srand(time(NULL));
  int i;
  for (i = 0; i < 256; i++) {
    password->x[i] = i;
  }

  // kruth's shuffle
  // bug: index i's value can be i.
  for (i = 0; i < 256; i++) {
    int v = rand() % 256; // 0 ~255
    swap(&password->x[i], &password->x[v]);
  }
  return password;
}

// encode
void encode_data(byte bytes[], int size, Password *encode_pass) {
  for (int i = 0; i < size; i++) {
    bytes[i] = encode_pass->x[bytes[i]];
  }
}

// decode
void decode_data(byte bytes[], int size, Password *encode_pass) {
  Password decode_pass = {};
  int i;
  for (i = 0; i < PASS_LENGTH; i++) {
    decode_pass.x[encode_pass->x[i]] = i;
  }
  for (i = 0; i < size; i++) {
    bytes[i] = decode_pass.x[bytes[i]];
  }
}