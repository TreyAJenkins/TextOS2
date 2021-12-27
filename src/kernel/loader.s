global loader:function  ; making entry point visible to linker
extern kmain            ; kmain is defined elsewhere

; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN equ  1<<0                   ; align loaded modules on page boundaries
MEMINFO     equ  1<<1                   ; provide memory map
VIDINFO     equ  1<<2                   ; Video stuff
FLAGS       equ  MODULEALIGN | MEMINFO  | VIDINFO  ; this is the Multiboot 'flag' field
MAGIC       equ    0x1BADB002           ; 'magic number' lets bootloader find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum required

section .__mbHeader ; to keep GRUB happy, this needs to be first; see linker.ld
align 4

MultiBootHeader:
   dd MAGIC
   dd FLAGS
   dd CHECKSUM
   dd 0, 0, 0, 0, 0
   dd 0 ; 0 = set graphics mode
   ;dd 800, 600, 32
   dd 0, 0, 32
   ;dd 1920,1080,32 ; zero means no preference

section .text
align 4

; reserve initial kernel stack space
STACKSIZE equ 0x4000

loader:
   mov esp, stack+STACKSIZE           ; set up the stack
   push stack                         ; pass the initial stack ESP0
   push eax                           ; pass Multiboot magic number
   push ebx                           ; pass Multiboot info structure

   call  kmain                       ; call the kernel

   cli
	.hang:
	   hlt                                ; halt machine should kernel return
	   jmp   .hang

section .bss
align 4
stack:
   resb STACKSIZE                     ; reserve 16k stack on a doubleword boundary
