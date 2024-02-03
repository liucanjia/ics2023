/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"

#define CHECKREG(reg) if (ref_r->reg != cpu.reg) { return false; }

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  int nr_gpr = MUXDEF(CONFIG_RVE, 16, 32);
  for (int i = 0; i < nr_gpr; i++) {
    if (ref_r->gpr[i] != gpr(i)) {
      return false;
    }
  }

  CHECKREG(pc);
  CHECKREG(mcause);
  CHECKREG(mstatus);
  CHECKREG(mepc);
  CHECKREG(mtvec);

  return true;
}

void isa_difftest_attach() {
}
