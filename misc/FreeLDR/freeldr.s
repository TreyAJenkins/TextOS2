k; Set DS (data segment base) as 0x7c0
mov ax, 0x7c0
mov ds, ax

mov cx, MSGLEN
mov si, msg
mov ah, 0xe ; BIOS 10h function code for tty output
putchar:
mov al, [si] ; Character to be displayed
int 0x10 ; BIOS interrupt for video service
inc si
loop putchar

jmp $ ; Jump here (i.e, loop forever)

msg: db 'TextOS FreeLDR'

; Let MSGLEN = Length of msg
MSGLEN: EQU ($ - msg)

; We need the boot signature as the last two bytes.
; That's why the remaining space is padded off.
padding: times (510 - ($ - $$)) db 0

BOOT_SIGN: db 0x55, 0xaa
