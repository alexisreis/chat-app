#include <stdio.h>
#include <openssl/sha.h>
#include <string.h>

int main (int argc, char** argv)
{
    if(argc != 2) return 1;

    char * password = argv[1];
    int length = strlen(argv[1]);

    printf("%s\n", password);
    printf("%d\n", length);

    SHA256_CTX context;
    unsigned char md[SHA256_DIGEST_LENGTH];

    SHA256_Init(&context);
    SHA256_Update(&context, (unsigned char*)password, length);
    SHA256_Final(md, &context);

    printf("%s\n", md);
    return 0;
}
/*
SHA256_CTX context;
unsigned char md[SHA256_DIGEST_LENGTH];

SHA256_Init(&context);
SHA256_Update(&context, (unsigned char*)input, length);
SHA256_Final(md, &context);
*/

