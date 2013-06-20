/*
 *
 */

.globl RtlCompareMemory
.globl RtlCompareMemoryUlong
.globl RtlFillMemory
.globl RtlFillMemoryUlong
.globl RtlFillMemoryUlonglong
.globl RtlMoveMemory
.globl RtlZeroMemory
.globl RtlPrefetchMemoryNonTemporal
        
RtlCompareMemory:
1:
        mr 0,5
        
        cmpwi 0,5,4
        blt 2f

        lwz 6,0(3)
        lwz 7,0(3)
        addi 6,6,-7
        cmpwi 0,6,0
        bne 2f

        addi 3,3,4
        addi 4,4,4
        subi 5,5,4
        b 1b
        
2:      
        cmpwi 0,5,0
        beq 3f
        
        lbz 6,0(3)
        lbz 7,0(4)
        addi 6,6,-7
        cmpwi 0,6,0
        bne 3f
        
        addi 3,3,1
        addi 4,4,1
        subi 5,5,1
        b 2b

3:
        mr 4,0
        sub 3,4,5
        blr
        
RtlCompareMemoryUlong:
        or 6,3,4
        or 6,6,5
        andi. 6,6,3
        bne RtlCompareMemory
        xor 3,3,3
        blr
        
RtlFillMemory:
        rlwinm 6,5,8,0xff00
        rlwinm 7,5,0,0xff
        or 7,6,7
        rlwinm 5,7,16,0xffff0000
        or 5,7,5
        
1:      
        cmpwi 0,4,4
        blt 2f

        stw 5,0(3)
        
        addi 3,3,4
        subi 4,4,4
        b 1b

2:
        cmpwi 0,4,0
        beq 3f

        stb 5,0(3)
        
        addi 3,3,1
        subi 4,4,1
        b 2b

3:
        blr
        
RtlFillMemoryUlong:
        b RtlFillMemory
        
RtlFillMemoryUlonglong:
        b RtlFillMemoryUlong

RtlMoveMemory:  
        b memmove
        
RtlZeroMemory:
        mr 5,4
        xor 4,4,4
        b memset

RtlPrefetchMemoryNonTemporal:
        blr
