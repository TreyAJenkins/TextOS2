// TextOS Nucleus Security Suite
#include <kernel/kernutil.h>
#include <kernel/timer.h>
#include <kernel/backtrace.h>
#include <kernel/console.h>
#include <string.h>
#include <stdint.h>
#include <kernel/crypto/aes.h>
#include <kernel/crypto/md5.h>
#include <kernel/crypto/sha2.h>

long AuthToken = 118999;
bool OpenToken = false;
bool VFuse = false;

bool FuseBlown(void) {
    return VFuse;
}
int RequestToken(void) {
    if (OpenToken && !FuseBlown()) {
        OpenToken = false;
        return AuthToken;
    } else {
        return 0;
    }
}
bool ValidateToken(int token) {
    if (!FuseBlown()) {
        if (token == AuthToken) {
            return true;
        } else {
            VFuse = true; //This function should NEVER be called by anything other than the kernel, so blow the fuse
            return false;
        }
    }
    return false;
}


//Partition: Random Number Generator
unsigned long seed = 7253; //Default seed, this will be overridden on boot
unsigned long random_seed = TIMER_HZ; //This will eventually store the last generated random number

void SetSeed(int token, long pseed) {
    if (ValidateToken(token)) {
        seed = pseed;
    }
}

int crand(int max) {
	random_seed = random_seed+seed * 1103515245 +12345;
	return (unsigned int)(random_seed / 65536) % (max+1);
}
//End Partition

//Partition: Stack Smashing Protection
#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void) {
    if (!OpenToken) {
        printk("\n-------------------------------------------\n");
        print_backtrace();
        panic("TExtOS Nucleus: STACK SMASHING DETECTED!");
    } else {
        OpenToken = false;
    }
}
//End Partition

//Partition: Init
int GenerateToken(void) {
    if (!FuseBlown()) {
        AuthToken = crand(4000000);
    }
    return 0;
}
int InitSSP(void) {
    return 0;
}
//End Partition

int test_decrypt_cbc(void) {

    uint8_t key[] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
                      0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4 };
    uint8_t in[]  = { 0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba, 0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6,
                      0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb, 0x80, 0x8d, 0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d,
                      0x39, 0xf2, 0x33, 0x69, 0xa9, 0xd9, 0xba, 0xcf, 0xa5, 0x30, 0xe2, 0x63, 0x04, 0x23, 0x14, 0x61,
                      0xb2, 0xeb, 0x05, 0xe2, 0xc3, 0x9b, 0xe9, 0xfc, 0xda, 0x6c, 0x19, 0x07, 0x8c, 0x6a, 0x9d, 0x1b };

    uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    uint8_t out[] = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
                      0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
                      0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
                      0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10 };
//  uint8_t buffer[64];
    struct AES_ctx ctx;

    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, in, 64);

    if (0 == memcmp((char*) out, (char*) in, 64)) {
        printk("SUCCESS!\n");
	return(0);
    } else {
        printk("FAILURE!\n");
	return(1);
    }
}

int test_md5(void) {
    char* str = "TextOS";
    uint8_t *result = md5String(str);

    printk("\n");
    for (size_t i = 0; i < 16; i++) {
        printk("%02x", result[i]);
    }
    printk("\n");
    

}

int test_sha2(void) {
    char* str = "TextOS";
    uint8_t digest[32];

    sha256(str, 6, digest);

    printk("\n");
    for (size_t i = 0; i < 32; i++) {
        printk("%02x", digest[i]);
    }
    printk("\n");
}

int test_python(void) {
    uint8_t raw[232] = { 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD3, 0xE4, 0xBA, 0xE0, 0xF6, 0xBE, 0xDF, 0xCE, 0x2F, 0x48, 0xE1, 0xED, 0x20, 0x77, 0xA3, 0xAE, 0x70, 0xAC, 0xAB, 0xD9, 0x90, 0x41, 0x03, 0xB6, 0x8A, 0x80, 0x97, 0x6C, 0x5F, 0x18, 0xD0, 0x9A, 0x84, 0x42, 0x64, 0x70, 0xFB, 0x48, 0xC1, 0xA8, 0x81, 0xAD, 0x3D, 0x12, 0xC5, 0x06, 0xEB, 0x80, 0xA7, 0x81, 0x37, 0x97, 0x66, 0xA7, 0x59, 0xE8, 0xE3, 0x55, 0x12, 0x8D, 0x08, 0x69, 0xEE, 0xD5, 0xC7, 0x16, 0xFD, 0x7D, 0xA7, 0x3D, 0x18, 0x13, 0x1F, 0x5C, 0xA0, 0xFC, 0x37, 0xDF, 0xF5, 0xC4, 0x8E, 0x5A, 0x3A, 0x6F, 0xD6, 0x07, 0xB5, 0x60, 0x7C, 0xC1, 0x5E, 0x46, 0xE7, 0x8E, 0xE6, 0xB8, 0x8A, 0x30, 0x5E, 0x84, 0x4B, 0x29, 0x09, 0x67, 0x7B, 0x83, 0xE4, 0xB4, 0x6E, 0x88, 0x7B, 0x8E, 0x1D, 0x5E, 0x48, 0x64, 0x04, 0x75, 0x9D, 0xB8, 0x11, 0xEC, 0xE0, 0x6B, 0x24, 0xBE, 0xA0, 0xF5, 0x70, 0x28, 0x62, 0x97, 0x11, 0xD8, 0x7A, 0x8F, 0xDA, 0xE1, 0x89, 0xF7, 0x57, 0x95, 0xAD, 0x65, 0x4C, 0x7F, 0x3C, 0xB6, 0xE3, 0x4B, 0x49, 0xE9, 0x68, 0x91, 0xD8, 0x1F, 0x1B, 0x0F, 0xBB, 0x2B, 0xFF, 0xD5, 0x4D, 0x5C, 0x47, 0x51, 0xA5, 0xDE, 0x5E, 0x55, 0x55, 0x5A, 0x3A, 0xF7, 0x18, 0xEB, 0xBB, 0x78, 0xDE, 0x97, 0x42, 0x7A, 0xAE, 0x5D, 0xD9, 0xAD, 0x9B, 0x7B, 0xD7, 0xA8, 0x2C, 0x95, 0x02, 0x3D, 0xA6, 0xE6, 0x03, 0x39, 0x5C, 0x4F, 0xF1, 0xB4, 0x4F, 0xB2, 0xD8, 0xD9, 0xE1, 0xD0, 0x38, 0x32, 0xFC, 0xCD, 0x23, 0x94, 0x7F, 0x32, 0x75, 0x29, 0x5F, 0x01, 0xD3, 0xD3, 0xCE, 0x4A, };    
    
    //uint8_t *key = md5String("TextOS");
    uint8_t key[32];
    sha256("TextOS", 6, key);

    long len = raw[0] | ( (int)raw[1] << 8 ) | ( (int)raw[2] << 16 ) | ( (int)raw[3] << 24 ) | ( (int)raw[4] << 32 ) | ( (int)raw[5] << 40 ) | ( (int)raw[6] << 48 ) | ( (int)raw[7] << 56 );

    long msgSize = len-16;
    printk("Size: %i\n", msgSize);

    uint8_t iv[16];
    memcpy(iv, raw+8, 16);
    uint8_t *in = raw+16+8;

    printk("\n");

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, in, msgSize);
    //for (size_t i = 0; i < 64; i++) {
    //    printk("%02x", in[i]);
    //}
    printk("\n");
    for (size_t i = 0; i < msgSize; i++) {
        printk("%c", in[i]);
    }


    printk("\n");
}

uint32 decrypt_initrd(uint32 initrd_address) {
    uint8_t key[32];
    sha256("TextOS", 6, key);
    
    uint8_t *raw = initrd_address;

    long len = raw[0] | ( (int)raw[1] << 8 ) | ( (int)raw[2] << 16 ) | ( (int)raw[3] << 24 ) | ( (int)raw[4] << 32 ) | ( (int)raw[5] << 40 ) | ( (int)raw[6] << 48 ) | ( (int)raw[7] << 56 );

    long msgSize = len-16;
    //printk("Size: %i\n", msgSize);    

    uint8_t iv[16];
    memcpy(iv, raw+8, 16);
    uint8_t *in = raw+16+8;

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, in, msgSize);

    //printk("Old Addr: %i\n", initrd_address);
    //printk("New Addr: %i\n", in);
    return in;
}