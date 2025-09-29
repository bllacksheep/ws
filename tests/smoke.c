#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "4443"
#define ADDR "127.0.0.1"
#define SERVERBIN "bin/server"

static pid_t server_pid;

void setup(void) {
  char *path = realpath(SERVERBIN, NULL);
  cr_assert(path != NULL, "error: realpath");

  pid_t pid = fork();
  cr_assert(pid != -1, "error: fork");

  if (pid == 0) {
    execl(path, "server", ADDR, PORT, (char *)NULL);
    // won't run if execl succeeds
    perror("error: exec");
    exit(1);
  } else {
    server_pid = pid;
    sleep(1);
  }
}

void teardown(void) {
  kill(server_pid, SIGTERM);
  int status;
  waitpid(server_pid, &status, 0);
  if (WIFEXITED(status)) {
    printf("exit status: %d", WEXITSTATUS(status));
  }
}

TestSuite(smoke, .init = setup, .fini = teardown, .disabled = 0);

Test(smoke, basic) { cr_assert(1, "hello"); }

// Test(smoke, siege) { cr_assert(1, "hello"); }
