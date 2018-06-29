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

void test_generate_password() {
  char string[] =
      "ooccDJX8EwaTL5I1JY9B4k1cc52XTxoQWSRSKZmBuBfLoG76qip8tnXpfQVesPlbO4hMx82L"
      "bOEr84yYdKtnMLwRMRvkZp4yWgPo4K08hNUtYnb+09KAz6/Jo843oaUAfpphxgG3te/"
      "epycPOjPsw2D1g1UJf44IVLmGpq7acpsuBxVoRcL4iWOfCj6sqYpH3MHY0LPx64K9tFhAHxh"
      "LwIV7Rv8/"
      "kOfFV04S8HekkWq+"
      "NFAU48TfyvL0XxYO5hlKegTI1ybdzFZpRO731rEC1EKUb7sjbT2oIB44sihwItFdQ/"
      "vZ7WtJcVPqHf2/SLp4IQ3lLJbbOXlRjQucNmRl9g==";
  log_d("Generate password by string:\n");

  Password *gen_pass = gen_password_by_string(string, strlen(string));
  for (int i = 0; i < PASS_LENGTH; i++) {
    printf("%d ", gen_pass->x[i]);
  }
  free(gen_pass);
}

/** // unit test
int main() {
  // test_generate_password();
  Password *pass = rand_password();
  for (int i = 0; i < PASS_LENGTH; i++) {
    printf("%d ", pass->x[i]);
  }
  printf("\n");

  log_d("Base64 string: \n");
  size_t size;
  char *base64 = string(pass, &size);
  printf("%s\n", base64);

  log_d("Generate password by string:\n");
  Password *gen_pass = gen_password_by_string(base64, size);
  for (int i = 0; i < PASS_LENGTH; i++) {
    printf("%d ", gen_pass->x[i]);
  }

  printf("\n");
  log_d("encode and decode data:");
  byte a[] = "hello";
  encode_data(a, sizeof(a), pass);
  puts(a);
  decode_data(a, sizeof(a), pass);
  puts(a);

  free(pass);
  free(gen_pass);
  free(base64);

  return 0;
}**/