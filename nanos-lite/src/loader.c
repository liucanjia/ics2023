#include <proc.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;
extern size_t ramdisk_read(void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr  ehdr;
  /* read the elf head */
  int ret = ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));
  assert(ret == sizeof(Elf_Ehdr));

  /* check if the file is an ELF image */
  assert(*(uint32_t *)ehdr.e_ident == 0x464C457F);

  /* ensure it is executable file */
  assert(ehdr.e_type == ET_EXEC);

  /* check the ISA type */
#if defined(__ISA_AM_NATIVE__) || defined(__ISA_X86_64__)
#define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86__)
#define EXPECT_TYPE EM_386
#elif defined(__ISA_MIPS32__)
#define EXPECT_TYPE EM_MIPS_RS3_LE
#elif defined(__riscv)
#define EXPECT_TYPE EM_RISCV
#elif defined(__ISA_LOONGARCH32R__)
#define EXPECT_TYPE EM_LOONGARCH
#else
#error Unsupported ISA
#endif
  assert(ehdr.e_machine == EXPECT_TYPE);

  /* read the program header table */
  Elf_Phdr phdr[ehdr.e_phnum];

  ret = ramdisk_read(phdr, ehdr.e_phoff, sizeof(Elf_Phdr) * ehdr.e_phnum);
  assert(ret == sizeof(Elf_Phdr) * ehdr.e_phnum);

  /* load the segments to memory */
  for (int i = 0; i < ehdr.e_phnum; i++) {
    if (phdr[i].p_type == PT_LOAD) {
      // set .bbs with zero
      memset((void*)phdr[i].p_vaddr, 0, phdr[i].p_memsz);
      ramdisk_read((void*)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_filesz);
    }
  }

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

