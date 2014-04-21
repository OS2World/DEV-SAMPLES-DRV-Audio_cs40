;
; segsetup.asm, math support
; 15-Jan-99
; watcom.c 16-bit
; wasm setsetup.asm

.386
.seq

public _endData
public _endHeap
public _endInitData
public _endCode

_HEADER         segment para public use16 'DATA'
_HEADER         ends

_DATA           segment para public use16 'DATA'
_DATA           ends

CONST           segment para public use16 'DATA'
CONST           ends

CONST2          segment para public use16 'DATA'
CONST2          ends

_BSS            segment para public use16 'BSS'
_BSS            ends

;_ENDDS          segment para public use16 'ENDDS'
;_endData        dw 0
;_ENDDS          ends
;
;_HEMP           segment dword public use16 'ENDDS'  ;named HEMP so can use _endHeap
;_HEMP           ends
;
;_ENDHEMP        segment dword public use16 'ENDDS'
;_endHeap        dw 0
;_ENDHEMP        ends

_HEMP           segment dword public use16 'BSS'  ;named HEMP so can use _endHeap
_HEMP           ends                              ;in BSS so doesn't bloat .sys

_ENDHEMP        segment dword public use16 'BSS'
_endHeap        dw 0
_ENDHEMP        ends

_ENDDS          segment para public use16 'ENDDS'
_endData        dw 0
_ENDDS          ends


_INITDATA       segment para public use16 'ENDDS'
_INITDATA       ends

_ENDINITDATA    segment para public use16 'ENDDS'
_endDataInit    dw 0
_ENDINITDATA    ends

_TEXT           segment para public use16 'CODE'
_TEXT           ends

_ENDCS          segment para public use16 'CODE'
_endCode        dw 0
_ENDCS          ends

RMCODE          segment para public use16 'CODE'
RMCODE          ends

_INITTEXT       segment para public use16 'CODE'
_INITTEXT       ends


DGROUP          group _HEADER, CONST, CONST2, _DATA, _BSS, _ENDDS, _HEMP, _ENDHEMP, _INITDATA, _ENDINITDATA
CGROUP          group _TEXT, _ENDCS, RMCODE, _INITTEXT

; -------------------------------------------------------------
; and any needed asm (might as well just use the one .asm file)

extrn DOSIODELAYCNT:ABS     ;DOSCALLS.427

public iodelay_

public __U4M
public __I4M
public __U4D
public __I4D

; --------------------------------------------------------------------
; in: cx = times 500 us (cx=2 then waits 1 uS (1 millionth of a second)
;out: n/a
;nts: both _iodelay and iodelay_ available

_TEXT   segment para PUBLIC USE16 'CODE'
        assume cs:cgroup, ds:dgroup

_iodelay label near
iodelay_ proc near

        db 0B8h                 ;mov ax,
        dw DOSIODELAYCNT        ;wordcount
ALIGN 4
@@:     dec   ax
        jnz   @b
        loop  iodelay_
        ret

iodelay_ endp

_TEXT   ENDS


;; Long multiply routine
;;
;; Arguments
;;    DX:AX * CX:BX
;; Returns
;;    DX:AX = product
;; Notes
;;    Trashes high words of 32-bit registers EAX and EDX

_TEXT   segment para PUBLIC USE16 'CODE'
        assume cs:cgroup, ds:dgroup

__U4M   proc  near
__I4M:  shl   edx,10h            ;; Load dx:ax into eax
        mov   dx,ax
        mov   eax,edx
        mov   dx,cx              ;; Load cx:bx into edx
        shl   edx,10h
        mov   dx,bx
        mul   edx                ;; Multiply eax*edx into edx:eax
        mov   edx,eax            ;; Load eax into dx:ax
        shr   edx,10h
        ret
__U4M   endp

_TEXT   ENDS

;; Long unsigned divide routine
;;
;; Arguments
;;    DX:AX / CX:BX
;; Returns
;;    DX:AX = quotient
;;    CX:BX = remainder
;; Notes
;;    Trashes high words of 32-bit registers EAX, ECX and EDX

_TEXT   segment para PUBLIC USE16 'CODE'
        assume cs:cgroup, ds:dgroup

__U4D   proc  near
        shl   edx,10h            ;; Load dx:ax into eax
        mov   dx,ax
        mov   eax,edx
        xor   edx,edx            ;; Zero extend eax into edx
        shl   ecx,10h            ;; Load cx:bx into ecx
        mov   cx,bx
        div   ecx                ;; Divide eax/ecx into eax
        mov   ecx,edx            ;; Load edx into cx:bx
        shr   ecx,10h
        mov   bx,dx
        mov   edx,eax            ;; Load eax into dx:ax
        shr   edx,10h
        ret
__U4D   endp

_TEXT   ENDS

;; Long signed divide routine
;;
;; Arguments
;;    DX:AX / CX:BX
;; Returns
;;    DX:AX = quotient
;;    CX:BX = remainder
;; Notes
;;    Trashes high words of 32-bit registers EAX, ECX and EDX

_TEXT   segment para PUBLIC USE16 'CODE'
        assume cs:cgroup, ds:dgroup

__I4D   proc  near
        shl   edx,10h            ;; Load dx:ax into eax
        mov   dx,ax
        mov   eax,edx
        cdq                      ;; Sign extend eax into edx
        shl   ecx,10h            ;; Load cx:bx into ecx
        mov   cx,bx
        idiv  ecx                ;; Divide eax/ecx into eax
        mov   ecx,edx            ;; Load edx into cx:bx
        shr   ecx,10h
        mov   bx,dx
        mov   edx,eax            ;; Load eax into dx:ax
        shr   edx,10h
        ret
__I4D   endp

_TEXT   ENDS

        end


