#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

static void sh_handle_cmd(const char *cmd) {
  if (cmd == NULL) return;

  char fname_buf[128];
  if (strncmp(cmd, "echo", strlen("echo")) == 0) {
    if (strlen(cmd) == strlen("echo") + 1) sh_printf("\n");
    else sh_printf("%s", cmd + strlen("echo "));
  } else {
    if (strlen(cmd) > sizeof(fname_buf)) {
      sh_printf("command too long\n");
      return;
    }

    strncpy(fname_buf, cmd, strlen(cmd));
    fname_buf[strlen(cmd) - 1] = '\0';
    
    setenv("PATH", "/bin", 0);
    execvp(fname_buf, NULL);
  }
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
