global loader
global gdt_flush
global idt_flush
global write_ident
global read_ident
global idt_blank_handler
global flagRND
global bootstrap_before
global bootstrap_after

extern main
extern oldESP
extern isr_handler

; Multiboot Header
MODULEALIGN equ 1<<0
MEMINFO equ 1<<1
FLAGS equ MODULEALIGN | MEMINFO
MAGIC equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .mbheader
align 4
MultiBootHeader:
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text

loader:
	mov esp, $stacktop
    cli
    call main
    cli

gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
.flush:
    ret

idt_flush:
    mov eax, [esp+4] 
    lidt [eax]
    ret
	
write_ident:
    mov eax, [esp+4] 
    mov cr2, eax
    ret
	
read_ident:
    mov eax, cr2
    ret

flagRND:
    mov eax, 1
    mov ecx, 0
    cpuid
    shr ecx, 30
    and ecx, 1
    mov eax, ecx
    ret

idt_blank_handler:
	pusha
	mov al, 0x20
	mov dx, 0x20
	out dx, al
	popa
	iret

%macro ISR_NOERR 1
	global _isr%1
	_isr%1:
		cli
		push byte 0
		push byte %1
		jmp isr_common_handler
%endmacro

%macro ISR_ERR 1
	global _isr%1
	_isr%1:
		cli
		push byte %1
		jmp isr_common_handler
%endmacro

isr_common_handler:
	pushad
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov [oldESP], esp
	mov eax, esp
	push eax
	call isr_handler
	pop eax
	mov eax, [oldESP]
	mov esp, eax
	pop gs
	pop fs
	pop es
	pop ds
	popad
	add esp, 8
	iret

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
ISR_NOERR 37
ISR_NOERR 38
ISR_NOERR 39
ISR_NOERR 40
ISR_NOERR 41
ISR_NOERR 42
ISR_NOERR 43
ISR_NOERR 44
ISR_NOERR 45
ISR_NOERR 46
ISR_NOERR 47

section .bss
align 4
STACKSIZE equ 0x1000
stack:
    resb STACKSIZE
stacktop: