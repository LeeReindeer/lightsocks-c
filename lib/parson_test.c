#include "parson.h"
#include <stdio.h>
#include <stdlib.h>

void test_parse_file() {
  JSON_Value *schema =
      json_parse_string("{\"local_port\":0,\"server_addr\":\"\",\"server_"
                        "port\":0,\"password\":\"\"}");
  JSON_Value *data = json_parse_file("test.json");
  char buf[256];
  int local_port, server_port;
  const char *server_addr, *password;
  if (data == NULL || json_validate(schema, data) != JSONSuccess) {
    printf("ERROR\n");
  }

  local_port = json_object_get_number((json_object(data)), "local_port");
  server_addr = json_object_get_string((json_object(data)), "server_addr");
  server_port = json_object_get_number(json_object(data), "server_port");
  password = json_object_get_string((json_object(data)), "password");

  printf("%d\n", local_port);
  puts(server_addr);
  printf("%d\n", server_port);
  puts(password);

  json_value_free(schema);
  json_value_free(data);
}

void test_serialize_to_file() {
  JSON_Value *root_val = json_value_init_object();
  JSON_Object *root_obj = json_value_get_object(root_val);
  json_object_set_number(root_obj, "local_port", 2333);
  json_object_set_string(root_obj, "server_addr", "1.1.1.1");
  json_object_set_number(root_obj, "server_port", 42619);
  json_object_set_string(
      root_obj, "password",
      "ooccDJX8EwaTL5I1JY9B4k1cc52XTxoQWSRSKZmBuBfLoG76qip8tnXpfQVesPlbO4hMx82L"
      "bOEr84yYdKtnMLwRMRvkZp4yWgPo4K08hNUtYnb+09KAz6/Jo843oaUAfpphxgG3te/"
      "epycPOjPsw2D1g1UJf44IVLmGpq7acpsuBxVoRcL4iWOfCj6sqYpH3MHY0LPx64K9tFhAHxh"
      "LwIV7Rv8/"
      "kOfFV04S8HekkWq+"
      "NFAU48TfyvL0XxYO5hlKegTI1ybdzFZpRO731rEC1EKUb7sjbT2oIB44sihwItFdQ/"
      "vZ7WtJcVPqHf2/SLp4IQ3lLJbbOXlRjQucNmRl9g==");
  json_serialize_to_file_pretty(root_val, "out.json");

  json_value_free(root_val);
}

int main() {
  test_serialize_to_file();
  test_parse_file();
  return 0;
}