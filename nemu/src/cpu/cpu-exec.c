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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10
#ifdef CONFIG_ITRACE
#define MAX_IRINGBUF_SIZE 32
#endif

extern void wp_difftest();
extern struct func_info *func_table;
extern size_t func_table_size;

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;
static struct ret_info *ret_list = NULL;
static int func_call_depth = 0;

#ifdef CONFIG_ITRACE
struct ibuf {
  char logbuf[128];
} iringbuf[MAX_IRINGBUF_SIZE];
size_t g_iringbuf_idx = -1;
#endif

void device_update();

#ifdef CONFIG_ITRACE
static void trace_iringbuf(Decode *_this) {
  g_iringbuf_idx = (g_iringbuf_idx + 1) % MAX_IRINGBUF_SIZE;
  strcpy(iringbuf[g_iringbuf_idx].logbuf, _this->logbuf);
}
#endif

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

  IFDEF(CONFIG_WATCHPOINT, wp_difftest());
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++;
    IFDEF(CONFIG_ITRACE, trace_iringbuf(&s));
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

#ifdef CONFIG_ITRACE
static void display_iringbuf() {
  if (nemu_state.state == NEMU_ABORT || (nemu_state.state == NEMU_END && nemu_state.halt_ret != 0)) {
    for (size_t i = 0; i < MAX_IRINGBUF_SIZE; i++) {
      if (i == g_iringbuf_idx) {
        printf("--> ");
      } else {
        printf("    ");
      }
      printf("%s\n", iringbuf[i].logbuf);
    }
  }
}
#endif

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      IFDEF(CONFIG_ITRACE, display_iringbuf());

      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT:
      if (func_table != NULL) {
        free(func_table);
      }
      statistic();
  }
}

/* trace the func call and ret */
void func_trace_call(vaddr_t pc, vaddr_t target, bool tail_call) {
  if (func_table == NULL) {
    return ;
  }

  int idx = find_func_name(target);
  _Log("0x%08lx:%*s call [%s@0x%08lx]\n", pc, func_call_depth, "", func_table[idx].func_name, target);

  if (tail_call == true) {
    ret_list_inster(pc);
  }

  func_call_depth++;
}

void func_trace_ret(vaddr_t pc) {
  if (func_table == NULL) {
    return ;
  }

  func_call_depth--;

  int idx = find_func_name(pc);
  _Log("0x%08lx:%*s ret  [%s]\n", pc, func_call_depth, "", func_table[idx].func_name);
  
  struct ret_info *node = ret_list->next;
  if (node != NULL && node->depth == func_call_depth) {
    vaddr_t tmp_addr = node->addr;
    ret_list_remove();
    func_trace_ret(tmp_addr);
  }
}

void ret_list_init() {
  ret_list = (struct ret_info*)malloc(sizeof(struct ret_info));
  ret_list->depth = 0;
  ret_list->addr = 0;
  ret_list->next = NULL;
}

void ret_list_inster(vaddr_t addr) {
  if (ret_list == NULL) {
    ret_list_init();
  }

  struct ret_info *tmp = (struct ret_info*)malloc(sizeof(struct ret_info));
  tmp->addr = addr;
  tmp->depth = func_call_depth;
  tmp->next = ret_list->next;
  ret_list->next = tmp;
}

void ret_list_remove() {
  struct ret_info *tmp = ret_list->next;

  if (tmp != NULL) {
    ret_list->next = tmp->next;
    free(tmp);
  } else {
    // only head node
    free(ret_list);
  }
}

int find_func_name(vaddr_t addr) {
  size_t idx = 0;

  for(; idx < func_table_size - 1; idx++) {
    if (addr >= func_table[idx].func_start && addr < func_table[idx].func_end) {
      return idx;
    }
  }

  return idx;
}
