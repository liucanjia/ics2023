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

#include "sdb.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
void wp_difftest() {
  for (WP *p = head; p != NULL; p = p->next) {
    bool success = true;
    word_t new = expr(p->buf, &success);
    if (p->old != new) {
      printf("Watchpoints %d: %s\n", p->NO, p->buf);
      printf("Old value: %lu\n", p->old);
      printf("New value: %lu\n", new);
    }
    p->old = new;
  }
}

WP* new_wp() {
  if (free_ != NULL) {
    WP* tmp = free_;
    free_ = free_->next;

    memset(tmp, 0, sizeof(WP));
    tmp->next = head;
    head = tmp;
    return tmp;
  }

  panic("No enough free watchpoint!");
  return NULL;
}

void free_wp(WP *wp) {
  WP *pre = NULL, *curr = head;
  
  for (; curr != NULL; pre = curr, curr = curr->next) {
    if (curr == wp) {
      break;
    }
  }

  if (pre != NULL) {
    pre->next = curr->next;
  } else {
    head = curr->next;
  }

  curr->next = free_;
  free_ = curr;
}

WP* find_wp(int no) {
  for (WP* p = head; p != NULL; p = p->next) {
    if (p->NO == no) {
      return p;
    }
  }

  return NULL;
}

void wp_display() {
  WP *p = head;

  if (p == NULL) {
    printf("No watchpoints!\n");
  } else {
    printf("%-8s%-8s\n", "No", "EXPR");
    while (p != NULL) {
      printf("%-8d%-8s\n", p->NO, p->buf);
      p = p->next;
    }
  }
}
