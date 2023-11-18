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
#include "memory/vaddr.h"
#include "memory/paddr.h"

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ, TK_AND, TK_OR, TK_GRE_EQ, TK_LESS_EQ, TK_INT, TK_REG, TK_VAR, TK_POSITIVE, TK_NEGATIVE, TK_DEREF

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
  {"%%", '%'},          // mod
  {"\\(", '('},         // left parentheses
  {"\\)", ')'},         // right parentheses
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_AND},       // and
  {"\\|\\|", TK_OR},    // or
  {"!", '!'},           // not
  {">", '>'},           // greater 
  {">=", TK_GRE_EQ},    // greater or equal to
  {"<", '<'},           // less
  {"<=", TK_LESS_EQ},   // less than or equal to
  {"&", '&'},           // bitwise and
  {"\\|", '|'},         // bitwise or
  {"\\^", '^'},         // bitwise xor
  {"~", '~'},           // bitwise inversion
  {"(0x)?[0-9]+", TK_INT},   // decimal or hexadecimal number
  {"[\\$][a-z0-9]{1,3}", TK_REG}, // register
  {"[A-Za-z_]\\w*", TK_VAR} // variable
};

static struct op_rank {
  int token_type;
  int rank;
} op_ranks[] = {
  {TK_NEGATIVE, 2},
  {TK_POSITIVE, 2},
  {TK_DEREF, 2},
  {'!', 2},
  {'~', 2},
  {'*', 3},
  {'/', 3},
  {'%', 3},
  {'+', 4},
  {'-', 4},
  {'>', 6},
  {TK_GRE_EQ, 6},
  {'<', 6},
  {TK_LESS_EQ, 6},
  {TK_EQ, 7},
  {TK_NEQ, 7},
  {'&', 8},
  {'^', 9},
  {'|', 10},
  {TK_AND, 11},
  {TK_OR, 12},
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
static bool switch_to_unary_operator();
static word_t calc1(int op, word_t val2, bool *success);
static word_t calc2(word_t val1, int op, word_t val2, bool *success);
static int get_op_rank(int op);
static bool is_unary_operator(int op);

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
          case '+':
            if (switch_to_unary_operator() == true) {
              /* positive number */
              tokens[nr_token].type = TK_POSITIVE;
            }
            break;
          case '-':
            if (switch_to_unary_operator() == true) {
              /* negative number */
              tokens[nr_token].type = TK_NEGATIVE;
            }
            break;
          case '*':
            if (switch_to_unary_operator() == true) {
              /* "*" mean dereference */
              tokens[nr_token].type = TK_DEREF;
            }
            break;
          case '/': case '%': case '(': case ')': case TK_EQ: case TK_NEQ: case TK_AND: case TK_OR: case '!': case '>': case TK_GRE_EQ: case '<': case TK_LESS_EQ: case '&': case '|': case '^': case '~':
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
    word_t val = 0;
    switch (tokens[start].type) {
      case TK_INT:
        /* <expr> ::= <decimal or hexadecimal number> */
        if (strlen(tokens[start].str) > 2 && tokens[start].str[1] == 'x') {
          sscanf(tokens[start].str, "%lx", &val);
        } else {
          sscanf(tokens[start].str, "%ld", &val);
        }
        break;
      case TK_REG:
        val = isa_reg_str2val(tokens[start].str, success);
        break;
      case TK_VAR:
        TODO();
        break;
      default:
        *success = false;
    }
    return val;
  } else if (check_parentheses(start, end) == false)  {
    /* parentheses is illegal */
    *success = false;
    return 0;
  } else if (is_big_parentheses(start, end) == true) {
    /* <expr> ::= "(" <expr> ")" */
    return eval(start + 1, end - 1, success);
  } else {
    /*  <expr> ::= <expr> "+" <expr>
          | <expr> "-" <expr>
          | <expr> "*" <expr>
          | <expr> "/" <expr>
          | <expr> "%" <expr>
          | <expr> "==" <expr>
          | <expr> "!=" <expr>
          | <expr> "&&" <expr>
          | <expr> "||" <expr>
          | <expr> ">" <expr>
          | <expr> ">=" <expr>
          | <expr> "<" <expr>
          | <expr> "<=" <expr>
          | <expr> "&" <expr>
          | <expr> "|" <expr>
          | <expr> "^" <expr>
          | "~" <expr>
          | "+" <expr>
          | "-" <expr>
          | "*" <expr>              # dereference pointer
    */
    int op = find_primary_op(start, end);

    bool ok1 = true, ok2 = true;
    word_t val1 = eval(start, op - 1, &ok1);
    word_t val2 = eval(op + 1, end, &ok2);

    if (ok2 == false) {
      *success = false;
    }

    if (ok1 == true) {
      return calc2(val1, tokens[op].type, val2, success);
    } else {
      return calc1(tokens[op].type, val2, success);
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
  int op_pos = -1, op_rank = 0;

  for (int i = start; i <= end; i++) {
    if (left_parentheses > 0) {
      if (tokens[i].type == '(') {
        left_parentheses++;
      } else if (tokens[i].type == ')') {
        left_parentheses--;
      }
      continue;
    }

    if (tokens[i].type == '(') {
      left_parentheses++;
      continue;
    }

    int cur_op_rank = get_op_rank(tokens[i].type);
    if (cur_op_rank > op_rank || (cur_op_rank == op_rank && is_unary_operator(tokens[i].type) == false)) {
      op_pos = i;
      op_rank = cur_op_rank;
    }

  }
  return op_pos;
}

static bool switch_to_unary_operator() {
  if (nr_token == 0) {
    return true;
  }

  switch (tokens[nr_token-1].type) {
    case ')':
    case TK_INT:
    case TK_REG:
    case TK_VAR:
      return false;
    default:
      return true;
  }
  return false;
}

static word_t calc1(int op, word_t val2, bool *success) {
  switch (op) {
    case '!':
      /* <expr> ::= "!" <expr> */
      return !val2;
    case '~':
      /* <expr> ::= "~" <expr> */
      return ~val2;
    case TK_POSITIVE:
      /* <expr> ::= + <number> */
      return val2;
    case TK_NEGATIVE:
      /* <expr> ::= - <number> */
      return -val2;
    case TK_DEREF:
      /* <expr> ::= "*" <expr> */
      if (likely(in_pmem(val2))) {
        return vaddr_read(val2, 8);
      } else {
        *success = false;
        break;
      }
    default:
      *success = false;
  }
  return 0;
}

static word_t calc2(word_t val1, int op, word_t val2, bool *success) {
  switch (op) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/': return (sword_t)val1 / (sword_t)val2;
    case '%': return val1 % val2;
    case TK_EQ: return val1 == val2;
    case TK_NEQ: return val1 != val2;
    case TK_AND: return val1 && val2;
    case TK_OR: return val1 || val2;
    case '>': return val1 > val2;
    case TK_GRE_EQ: return val1 >= val2;
    case '<': return val1 < val2;
    case TK_LESS_EQ: return val1 <= val2;
    case '&': return val1 & val2;
    case '|': return val1 | val2;
    case '^': return val1 ^ val2;
    default:
      *success = false;
  }

  return 0;
}

static int get_op_rank(int op) {
  for (int i = 0; i < sizeof(op_ranks)/sizeof(op_ranks[0]); i++) {
    if (op == op_ranks[i].token_type) {
      return op_ranks[i].rank;
    }
  }
  return 0;
}

static bool is_unary_operator(int op) {
  switch (op) {
    case TK_POSITIVE:
    case TK_NEGATIVE:
    case TK_DEREF:
    case '!':
    case '~':
      return true;
  }
  return false;
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