#ifndef __PASSWORD_H__
#define __PASSWORD_H__

#include <stddef.h>
#define PASS_LENGTH 256
// 0 ~255
#define byte unsigned char
/**
 * Password is a 256 length array,
 * use index and value as a table.
 **/
typedef struct {
  byte x[PASS_LENGTH];
} Password;

// represent password as string encode in base64
char *password_string(Password *pass, size_t *outsize);

// generate password by string encode in base64
Password *gen_password_by_string(char *string, size_t len);

Password *rand_password();

// encode
void encode_data(byte bytes[], int size, Password *pass);

// decode
void decode_data(byte bytes[], int size, Password *pass);

#endif // MACRO