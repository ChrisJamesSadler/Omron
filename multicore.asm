section .apentry
mov esp, end
mov ax,cs
mov ds,ax
mov es,ax

;mov al,03h
;mov ah,0
;int 10h

cli
mov dword [GDT_Pointer + 2], GDT_Contents
mov dword eax, GDT_Pointer
lgdt [eax]
mov ax, 0x10
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
jmp 0x08:flush
flush:
mov eax, cr0
or eax, 0x1
mov cr0, eax
sti

[bits 32]
mov dword eax, 103b64h
call eax
[bits 16]

GDT_Contents:
db 0, 0, 0, 0, 0, 0, 0, 0			; Offset: 0  - Null selector - required 
db 255, 255, 0, 0, 0, 0x9A, 0xCF, 0	; Offset: 8  - KM Code selector - covers the entire 4GiB address range
db 255, 255, 0, 0, 0, 0x92, 0xCF, 0	; Offset: 16 - KM Data selector - covers the entire 4GiB address range
db 255, 255, 0, 0, 0, 0xFA, 0xCF, 0	; Offset: 24 - UM Code selector - covers the entire 4GiB address range
db 255, 255, 0, 0, 0, 0xF2, 0xCF, 0	; Offset: 32 - UM Data selector - covers the entire 4GiB address range
db 0x67,  0, 0, 0, 0, 0xE9, 0x00, 0	; Offset: 40 - TSS Selector - Pointer to the TSS 
GDT_Pointer db 47, 0, 0, 0, 0, 0

end times 512-($-$$) db 0