/* { dg-require-effective-target arm_v8_1m_mve_fp_ok } */
/* { dg-add-options arm_v8_1m_mve_fp } */
/* { dg-additional-options "-O2" } */

#include "arm_mve.h"

void
foo (uint32x4_t * addr, const int offset, float32x4_t value)
{
  vstrwq_scatter_base_wb_f32 (addr, 8, value);
}

void
foo1 (uint32x4_t * addr, const int offset, float32x4_t value)
{
  vstrwq_scatter_base_wb (addr, 8, value);
}

/* { dg-final { scan-assembler-times "vstrw.u32\tq\[0-9\]+, \\\[q\[0-9\]+, #\[0-9\]+\\\]!" 2 } } */
