.global _entry
.global main
.global _sim_end
.global write_done
.global miserable_failure

.text

trap_handle:
  lui a2, 0x3
  mret

_entry:
  nop
main:
  li t1, (1 << 17) | (1 << 3) // mstatus.MPRV = 1, mstatus.MIE = 1
  csrs mstatus, t1
  la t1, trap_handle
  csrw mtvec, t1
  lui s2, 0x1
  lui s3, 0x2
  lui s4, 0x2
  lui s5, 0x2
  lui a6, 0x2
  sub s2, s2, s3
  lui s3, 0x88888

_sim_end:
write_done:
  j write_done

miserable_failure:
  j miserable_failure
