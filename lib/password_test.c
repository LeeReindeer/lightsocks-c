#include "log.h"
#include "password.h"
#include <stdlib.h>
#include <string.h>

void test_generate_password_by_string() {
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

void test_rand_password(Password *pass) {
  log_d("Base64 string: \n");
  size_t size;
  char *base64 = password_string(pass, &size);
  printf("%s\n", base64);

  log_d("Generate password by string:\n");
  Password *gen_pass = gen_password_by_string(base64, size);
  for (int i = 0; i < PASS_LENGTH; i++) {
    printf("%d ", gen_pass->x[i]);
  }
  for (int i = 0; i < PASS_LENGTH; i++) {
    check(gen_pass->x[i] == pass->x[i], "password not match")
  }
  printf("\n");

error:
  free(gen_pass);
  free(base64);
}

void test_encode_decode(Password *pass) {
  log_d("encode and decode data:");
  byte a[] = "hello";
  encode_data(a, sizeof(a), pass);
  puts(a);
  decode_data(a, sizeof(a), pass);
  puts(a);
}

// unit test
// gcc password_test.c password.o log.o base64.o -o test
int main() {
  Password *pass = rand_password();
  for (int i = 0; i < PASS_LENGTH; i++) {
    printf("%d ", pass->x[i]);
  }
  printf("\n");
  test_generate_password_by_string();
  test_rand_password(pass);
  test_encode_decode(pass);

  free(pass);

  log_d("All test pass");
  return 0;
}