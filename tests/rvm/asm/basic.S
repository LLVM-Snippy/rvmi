.global _entry
.global main
.global _sim_end
.global write_done
.global miserable_failure

.text
_entry:
  nop
main:
  li sp, ((1 << 9) | (1 << 13)) // mstatus.VS = 1 (initial), mstatus.FS = 1
  csrs  mstatus, sp
  fcvt.s.w ft0,  sp
  li t1, 0x7f
  li t2, 0x80001000
  sb t1, (t2)
  li t1, 0x7fff
  sh t1, 8(t2)
  li t1, 0x7fffffff
  sw t1, 16(t2)
  li t1, 0x7fffffffffffffff
  sd t1, 24(t2)

_sim_end:
write_done:
  j write_done

miserable_failure:
  j miserable_failure

