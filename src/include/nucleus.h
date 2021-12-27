#ifndef _NUCLEUS_H
#define _NUCLEUS_H

bool FuseBlown(void);
int RequestToken(void);
bool ValidateToken(int token);

void SetSeed(int token, unsigned long seed);
int crand(int max);

int GenerateToken(void);
int InitSSP(void);

int test_decrypt_cbc(void);
int test_md5(void);

uint32 decrypt_initrd(uint32 initrd_address);
#endif
