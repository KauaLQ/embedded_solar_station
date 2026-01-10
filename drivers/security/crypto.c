#include "crypto.h"

#include <stdio.h>
#include <string.h>

#include "secrets.h"
#include "mbedtls/md.h"

#ifndef HMAC_SECRET
#error "HMAC_SECRET não definida. Crie o arquivo secrets.h baseado em secrets.example.h"
#endif

void hmac_sha256(const char *input, char *output_hex) {
    unsigned char hmac[32];
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info;

    info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 1);

    mbedtls_md_hmac_starts(&ctx,
        (const unsigned char *)HMAC_SECRET,
        strlen(HMAC_SECRET)
    );

    mbedtls_md_hmac_update(&ctx,
        (const unsigned char *)input,
        strlen(input)
    );

    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    // Converte para hexadecimal
    for (int i = 0; i < 32; i++) {
        sprintf(output_hex + (i * 2), "%02x", hmac[i]);
    }
    output_hex[64] = '\0';
}