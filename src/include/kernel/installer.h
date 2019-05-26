#ifndef _INSTALLER_H
#define _INSTALLER_H

int install_core(int disk, char* core, uint32 size);
int install_mbr(int disk, char* mbr);
int install_kernel(int disk, char* kernel, int size, int offset);
#endif
