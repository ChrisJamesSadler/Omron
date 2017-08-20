;mov ax,0x1112
;mov bl,0
;int 0x10
lgdt [$+1]
add al, al
cli
mov eax, cr0
inc ax
mov cr0, eax
jmp 0x18:PM 
db 0
dw 0xB200, 0x00CF
dw -1, 0, 0x9800, 0x00CF
[bits 32]
PM: 
    sti
    mov ds, ax
    mov dword eax, [0x1000]
    jmp eax
done:
    hlt
    jmp done