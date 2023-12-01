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
#include <memory/paddr.h>
#include <cpu/cpu.h>
#include <elf.h>

struct func_info *func_table = NULL;
size_t func_table_size = 0;

void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm(const char *triple);

static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
  // Log("Exercise: Please remove me in the source code and compile NEMU again.");
  // assert(0);
}

#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static char *elf_file = NULL;
static int difftest_port = 1234;

static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}

static void load_elf() {
  if (elf_file == NULL) {
    Log("No elf is given. Can npt build symbol table.");
    return ;
  }

  FILE *fp = fopen(elf_file, "rb");
  Assert(fp, "Can not open '%s'", elf_file);

  Elf64_Ehdr  ehdr;
  /* read the elf head */
  int ret = fread(&ehdr, sizeof(Elf64_Ehdr), 1, fp);
  assert(ret == 1);

  /* check if the file is an ELF image */
  if (ehdr.e_ident[0] != 0x7f || ehdr.e_ident[1] != 'E' || ehdr.e_ident[2] != 'L' || ehdr.e_ident[3] != 'F') {
    printf("%s is not an ELF file\n", elf_file);
    return ;
  }

  /* ensure it is executable file */
  if (ehdr.e_type != ET_EXEC) {
    printf("%s is not an executable\n", elf_file);
    return ;
  }

  /* read the head table */
  Elf64_Shdr *shdr = (Elf64_Shdr*)malloc(sizeof(Elf64_Shdr) * ehdr.e_shnum);
  if (shdr == NULL) {
    printf("shdr malloc failed\n");
    return ;
  }

  ret = fseek(fp, ehdr.e_shoff, SEEK_SET);
  if (ret != 0) {
    printf("set fp offset failed\n");
    free(shdr);
    return ;
  }

  ret = fread(shdr, sizeof(Elf64_Shdr), ehdr.e_shnum, fp);
  if (ret != ehdr.e_shnum) {
    printf("read Sections header table failed\n");
    free(shdr);
    return ;
  }

  /* read the section holds table */
  rewind(fp); // reset the fp in the begin of the file

  fseek(fp, shdr[ehdr.e_shstrndx].sh_offset, SEEK_SET);
  char shstrtab[shdr[ehdr.e_shstrndx].sh_size];
  ret = fread(shstrtab, sizeof(shstrtab), 1, fp);
  if (ret != 1) {
    printf("read the section holds table failed\n");
    free(shdr);
    return ;
  }

  /* read the string table */
  rewind(fp); // reset the fp in the begin of the file

  Elf64_Word string_table_index = 0;
  while (strcmp(&shstrtab[shdr[string_table_index].sh_name], ".strtab") != 0) {
    string_table_index++;
  }

  ret = fseek(fp, shdr[string_table_index].sh_offset, SEEK_SET);
  if (ret != 0) {
    printf("set fp offset failed\n");
    free(shdr);
    return ;
  }

  char string_table[shdr[string_table_index].sh_size];
  ret = fread(string_table, sizeof(string_table), 1, fp);
  if (ret != 1) {
    printf("read the string table failed\n");
    free(shdr);
    return ;
  }

  /*read the symbol table */
  rewind(fp); // reset the fp in the begin of the file

  Elf64_Word symbol_table_index = 0;
  while (shdr[symbol_table_index].sh_type != SHT_SYMTAB) {
    symbol_table_index++;
  }

  ret = fseek(fp, shdr[symbol_table_index].sh_offset, SEEK_SET);
  if (ret != 0) {
    printf("set fp offset failed\n");
    free(shdr);
    return ;
  }

  char symbol_table[shdr[symbol_table_index].sh_size];
  ret = fread(symbol_table, sizeof(symbol_table), 1, fp);
  if (ret != 1) {
    printf("read the symbol table failed\n");
    free(shdr);
    return ;
  }

  /* read the symbol which type is FUNC */
  for (Elf64_Sym *ptr = (Elf64_Sym*)symbol_table; (char*)ptr < symbol_table + sizeof(symbol_table); ptr++) {
    if ((ptr->st_info & 0xf) == STT_FUNC && ptr->st_size != 0) {
      func_table_size++;
    }
  }

  func_table_size++;
  func_table = (struct func_info*)malloc(sizeof(struct func_info) * func_table_size);
  int idx = 0;
  for (Elf64_Sym *ptr = (Elf64_Sym*)symbol_table; (char*)ptr < symbol_table + sizeof(symbol_table); ptr++) {
    if ((ptr->st_info & 0xf) == STT_FUNC && ptr->st_size != 0) {
      strcpy(func_table[idx].func_name, &string_table[ptr->st_name]);
      func_table[idx].func_start = ptr->st_value;
      func_table[idx].func_end = ptr->st_value + ptr->st_size;
      idx++;
    }
  }
  // ??? for the case if do not find the func name match
  strcpy(func_table[idx].func_name, "???");
  func_table[idx].func_start = 0;
  func_table[idx].func_end = 0;

  free(shdr);
  return ;
}

static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"elf"      , required_argument, NULL, 'e'},
    {"help"     , no_argument      , NULL, 'h'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:s:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'e': elf_file = optarg; break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1: img_file = optarg; return 0;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\t-e,--elf=FILE_ELF       read the FILE_ELF\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Set random seed. */
  init_rand();

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Read the elf file to get the symbol table. */
  load_elf();

  /* Initialize the simple debugger. */
  init_sdb();

#ifndef CONFIG_ISA_loongarch32r
  IFDEF(CONFIG_ITRACE, init_disasm(
    MUXDEF(CONFIG_ISA_x86,     "i686",
    MUXDEF(CONFIG_ISA_mips32,  "mipsel",
    MUXDEF(CONFIG_ISA_riscv,
      MUXDEF(CONFIG_RV64,      "riscv64",
                               "riscv32"),
                               "bad"))) "-pc-linux-gnu"
  ));
#endif

  /* Display welcome message. */
  welcome();
}
#else // CONFIG_TARGET_AM
static long load_img() {
  extern char bin_start, bin_end;
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}

void am_init_monitor() {
  init_rand();
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif
