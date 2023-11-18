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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "memory/vaddr.h"
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *agrs);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  {"si", "Execute next [N] instruction (after stopping)", cmd_si },
  {"info", "Display information about registers or watchpoints", cmd_info },
  {"x", "Dispaly [N] bytes of memory, starting at address [EXPR]", cmd_x },
  {"p", "Calculate the value of [EXPR]", cmd_p },
  {"w", "Set watchpoint at the memory address of [EXPR]", cmd_w},
  {"d", "Delete [No] watchpoint in the memory", cmd_d}
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int n = 0;

  if (arg == NULL || sscanf(arg, "%d", &n) != 1 || (arg = strtok(NULL, " ")) != NULL) {
    /* argument is illegal */
    printf("(nemu) Usage: si [N]\n");
  } else {
    cpu_exec(n);
  }

  return 0;
}

static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  char ch = 0;

  /* argument is illegal */
  if (arg == NULL || sscanf(arg, "%c", &ch) != 1 || (arg = strtok(NULL, " ")) != NULL) {
    printf("(nemu) Usage: info [r or w]\n");
    return 0;
  }

  switch (ch) {
    case 'r':
      /* display information about registers */
      isa_reg_display();
      break;
    case 'w':
      /* display information about watchpoints */
      wp_display();
      break;
    default:
      /* argument is illegal */
      printf("(nemu) Usage: info [r or w]\n");
      break;
  }
  return 0;
}

static int cmd_x(char *args) {
  char *arg = strtok(NULL, " ");
  int n = 0;
  word_t addr = 0;
  bool success = true;

  /* extract the first argument */
  if (arg == NULL || sscanf(arg, "%d", &n) != 1) {
    printf("(nemu) Usage: x [N] [EXPR]\n");
    return 0;
  } 

  arg = arg + strlen(arg) + 1;
  /* calculate expression to find address */
  addr = expr(arg, &success);
  if(success != true) {
    printf("EXPR error!\n");
    printf("(nemu) Usage: x [N] [EXPR]\n");
    return 0;
  }

  for (int i = 0; i < n; i++) {
    printf("0x%08lx: 0x%08lx\n", addr, vaddr_read(addr, 4)); 
    addr += 4;
  }

  return 0;
}

static int cmd_p(char *args) {
  bool success = true;

  word_t  value = expr(args, &success);

  if (success == true) {
    printf("%lu\n", value);
  } else {
    printf("EXPR error!\n");
    printf("(nemu) Usage: p [EXPR]\n");
  }

  return 0;
}

static int cmd_w(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  bool success = true;

  if (arg == NULL) {
    printf("(nemu) Usage: w [EXPR]\n");
  }

  word_t old = expr(args, &success);
  if (success != true) {
    printf("EXPR error!\n");
    printf("(nemu) Usage: p [EXPR]\n");
  } else {
    WP *p = new_wp();
    strncpy(p->buf, args, sizeof(p->buf));
    p->old = old;
    printf("Watchpoint %d: %s\n", p->NO, args);
  }
  return 0;
}

static int cmd_d(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int no = 0;

  if (arg == NULL || sscanf(arg, "%d", &no) != 1 || (arg = strtok(NULL, " ")) != NULL) {
    /* argument is illegal */
    printf("(nemu) Usage: d [No]\n");
  } else {
    WP *p = find_wp(no);
    if (p != NULL) {
      free_wp(p);
      printf("Delete watchpoint %d: %s\n", p->NO, p->buf);
    }
  }

  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* test math expression calcuation */
  // test_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
