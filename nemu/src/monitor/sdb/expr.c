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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_INT, TK_ADDR, TK_REG, TK_VAR, TK_NEGATIVE

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
  {"\\*", '*'},         // multiplication
  {"\\/", '/'},         // division
  {"\\(", '('},         // left parentheses
  {"\\)", ')'},         // right parentheses
  {"==", TK_EQ},        // equal
  {"[0-9]+", TK_INT},   // integer
  {"0x[0-9a-fA-F]+", TK_ADDR}, // address
  {"[\\$][a-z0-9]{1,3}", TK_REG}, // register
  {"[A-Za-z_]\\w*", TK_VAR} // variable
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static word_t eval(int start, int end, bool *success);
static bool is_big_parentheses(int start, int end);
static bool check_parentheses(int start, int end);
static int find_primary_op(int start, int end);

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if (rules[i].token_type == TK_NOTYPE) break;

        tokens[nr_token].type = rules[i].token_type;
        switch (rules[i].token_type) {
          case TK_INT:
          case TK_ADDR:
          case TK_REG:
          case TK_VAR:
            if (substr_len >= 32) {
              printf("token too long, should less than 32: %*.s\n", substr_len, substr_start);
              return false;
            } else {
              strncpy(tokens[nr_token].str, substr_start, substr_len);
              tokens[nr_token].str[substr_len] = '\0';
            }
            break;
          case '-':
            if (nr_token == 0 || (tokens[nr_token-1].type != TK_INT && tokens[nr_token-1].type != TK_ADDR && tokens[nr_token-1].type != TK_REG && tokens[nr_token-1].type != TK_VAR && tokens[nr_token-1].type != ')')) {
              /* negative number */
              tokens[nr_token].type = TK_NEGATIVE;
            }
            break;
          case '+': case '*': case '/': case '(': case ')':
            break;
          default:
            panic("token type %d is not implement\n", rules[i].token_type);
        }
        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  return eval(0, nr_token - 1, success);
}

static word_t eval(int start, int end, bool *success) {
  if (*success == false) {
    return 0;
  }

  if (start > end) {
    *success = false;
  } else if (start == end) {
    /* <expr> ::= <number> */
    word_t val = 0;
    switch (tokens[start].type) {
      case TK_INT:
        sscanf(tokens[start].str, "%ld", &val);
        break;
      case TK_ADDR:
        break;
      case TK_REG:
        break;
      case TK_VAR:
        break;
      default:
        *success = false;
    }
    return val;
  } else if (tokens[start].type == TK_NEGATIVE) {
    /* <expr> ::= - <number> */
    return -eval(start + 1, end, success);
  } 
  else if (check_parentheses(start, end) == false)  {
    /* parentheses is illegal */
    *success = false;
    return 0;
  } else if (is_big_parentheses(start, end) == true) {
    /* <expr> ::= "(" <expr> ")" */
    return eval(start + 1, end - 1, success);
  } else {
    /* <expr> ::= <expr> op <expr> */
    int op = find_primary_op(start, end);

    word_t val1 = eval(start, op - 1, success);
    word_t val2 = eval(op + 1, end, success);
    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;       
      case '*': return val1 * val2;
      case '/': return (sword_t)val1 / (sword_t)val2; // 生成用例为有符号除法, 面向用例修改
      default:
        *success = false;
    }
  }

  return 0;
}

static bool is_big_parentheses(int start, int end) {
  if (tokens[start].type != '(' || tokens[end].type != ')') return false;
  return check_parentheses(start + 1, end - 1);
}

static bool check_parentheses(int start, int end) {
  int left_parentheses_cnt = 0;

  left_parentheses_cnt = 0;
  for (int i = start; i <= end && left_parentheses_cnt >= 0; i++) {
    switch (tokens[i].type) {
      case '(':
        left_parentheses_cnt++;
        break;
      case ')':
        left_parentheses_cnt--;
        break;
      default:
        break;
    }
  }
  if (left_parentheses_cnt != 0) return false;

  return true;
}

static int find_primary_op(int start, int end) {
  int left_parentheses = 0;
  int op = -1;

  for (int i = start; i <= end; i++) {
    if (left_parentheses > 0) {
      if (tokens[i].type == '(') {
        left_parentheses++;
      } else if (tokens[i].type == ')') {
        left_parentheses--;
      }
      continue;
    }

    switch (tokens[i].type) {
      case '(':
        left_parentheses++;
        break;
      case ')':
        left_parentheses--;
        break;
      case '+':
      case '-':
        op = i;
      case '*':
      case '/':
        if (op == -1 || tokens[op].type == '*' || tokens[op].type == '/') 
          op = i;
        break;
      }
    }
    return op;
}

void test_expr() {
  const char *nemu_home = getenv("NEMU_HOME");
  if (nemu_home == NULL) {
    printf("Can not get NEMU_HOME enviroment variable.\n");
    return;
  }

  char file_path[128];
  snprintf(file_path, sizeof(file_path), "%s/%s", nemu_home, "tools/gen-expr/build/input");

  FILE *fp = fopen(file_path, "r");
  assert(fp != NULL);

  char *e = NULL;
  word_t correct_val;
  size_t len;
  ssize_t read;
  bool success = true;

  size_t n = 0;

  while (true) {
    if(fscanf(fp, "%lu ", &correct_val) == -1) {
      break;
    }
    read = getline(&e, &len, fp);
    e[read-1] = '\0';

    word_t val = expr(e, &success);

    assert(success);

    if(val != correct_val) {
      printf("%s\n", e);
      printf("expected: %lu, got: %lu\n", correct_val, val);
      assert(0);
    }
    n++;
  }

  fclose(fp);
  if (e != NULL)
    free(e);

  Log("expr test passed %zu", n);
} 