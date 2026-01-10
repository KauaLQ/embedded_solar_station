#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>

void hmac_sha256(const char *input, char *output_hex);

#endif