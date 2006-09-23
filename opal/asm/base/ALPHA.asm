	.set noreorder
	.set volatile
	.set noat
	.set nomacro
	.text
	.align 2
	.align 4
	.globl opal_atomic_mb
	.ent opal_atomic_mb
$opal_atomic_mb..ng:
opal_atomic_mb:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	mb
	.set	nomacro
	ret $31,($26),1
	.end opal_atomic_mb
	.align 2
	.align 4
	.globl opal_atomic_rmb
	.ent opal_atomic_rmb
$opal_atomic_rmb..ng:
opal_atomic_rmb:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	mb
	.set	nomacro
	ret $31,($26),1
	.end opal_atomic_rmb
	.align 2
	.align 4
	.globl opal_atomic_wmb
	.ent opal_atomic_wmb
$opal_atomic_wmb..ng:
opal_atomic_wmb:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	wmb
	.set	nomacro
	ret $31,($26),1
	.end opal_atomic_wmb
	.align 2
	.align 4
	.globl opal_atomic_cmpset_32
	.ent opal_atomic_cmpset_32
$opal_atomic_cmpset_32..ng:
opal_atomic_cmpset_32:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	1:  ldl_l $0, 0($16)            
     cmpeq $0, $17, $0        
     beq $0, 2f              
     mov $18, $0              
     stl_c $0, 0($16)            
     beq $0, 3f              
2:                           
3:   br 1b                   

	.set	nomacro
	addl $31,$0,$0
	ret $31,($26),1
	.end opal_atomic_cmpset_32
	.align 2
	.align 4
	.globl opal_atomic_cmpset_acq_32
	.ent opal_atomic_cmpset_acq_32
$opal_atomic_cmpset_acq_32..ng:
opal_atomic_cmpset_acq_32:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	1:  ldl_l $0, 0($16)            
     cmpeq $0, $17, $0        
     beq $0, 2f              
     mov $18, $0              
     stl_c $0, 0($16)            
     beq $0, 3f              
2:                           
3:   br 1b                   

	.set	nomacro
	addl $31,$0,$0
	.set	macro
	mb
	.set	nomacro
	ret $31,($26),1
	.end opal_atomic_cmpset_acq_32
	.align 2
	.align 4
	.globl opal_atomic_cmpset_rel_32
	.ent opal_atomic_cmpset_rel_32
$opal_atomic_cmpset_rel_32..ng:
opal_atomic_cmpset_rel_32:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	wmb
	1:  ldl_l $0, 0($16)            
     cmpeq $0, $17, $0        
     beq $0, 2f              
     mov $18, $0              
     stl_c $0, 0($16)            
     beq $0, 3f              
2:                           
3:   br 1b                   

	.set	nomacro
	addl $31,$0,$0
	ret $31,($26),1
	.end opal_atomic_cmpset_rel_32
	.align 2
	.align 4
	.globl opal_atomic_cmpset_64
	.ent opal_atomic_cmpset_64
$opal_atomic_cmpset_64..ng:
opal_atomic_cmpset_64:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	1:  ldq_l $0, 0($16)           
     cmpeq $0, $17, $0        
     beq $0, 2f              
     mov $18, $0              
     stq_c $0, 0($16)            
     beq $0, 3f              
2:                           
3:   br 1b                   

	.set	nomacro
	addl $31,$0,$0
	ret $31,($26),1
	.end opal_atomic_cmpset_64
	.align 2
	.align 4
	.globl opal_atomic_cmpset_acq_64
	.ent opal_atomic_cmpset_acq_64
$opal_atomic_cmpset_acq_64..ng:
opal_atomic_cmpset_acq_64:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	1:  ldq_l $0, 0($16)           
     cmpeq $0, $17, $0        
     beq $0, 2f              
     mov $18, $0              
     stq_c $0, 0($16)            
     beq $0, 3f              
2:                           
3:   br 1b                   

	.set	nomacro
	addl $31,$0,$0
	.set	macro
	mb
	.set	nomacro
	ret $31,($26),1
	.end opal_atomic_cmpset_acq_64
	.align 2
	.align 4
	.globl opal_atomic_cmpset_rel_64
	.ent opal_atomic_cmpset_rel_64
$opal_atomic_cmpset_rel_64..ng:
opal_atomic_cmpset_rel_64:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	wmb
	1:  ldq_l $0, 0($16)           
     cmpeq $0, $17, $0        
     beq $0, 2f              
     mov $18, $0              
     stq_c $0, 0($16)            
     beq $0, 3f              
2:                           
3:   br 1b                   

	.set	nomacro
	addl $31,$0,$0
	ret $31,($26),1
	.end opal_atomic_cmpset_rel_64
	.align 2
	.align 4
	.globl opal_sys_timer_get_cycles
	.ent opal_sys_timer_get_cycles
$opal_sys_timer_get_cycles..ng:
opal_sys_timer_get_cycles:
	.eflag 48
	.frame $30,0,$26,0
	.prologue 0
	.set	macro
	rpcc $0
	.set	nomacro
	addl $31,$0,$0
	ret $31,($26),1
	.end opal_sys_timer_get_cycles
	.section	.note.GNU-stack,"",@progbits
	.ident	"GCC: (GNU) 3.4.6 (Gentoo 3.4.6-r1, ssp-3.4.5-1.0, pie-8.7.9)"
