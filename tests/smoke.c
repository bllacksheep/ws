#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define SERVER "bin/server"

static pid_t server_pid;

void setup(void) {
  pid_t pid = fork();
  cr_assert(pid >= 0, "error: fork");
  if (pid == 0) {
    execl(SERVER, SERVER, (char *)NULL);
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
  waitpid(server_pid, NULL, 0);
}

TestSuite(smoke, .init = setup, .fini = teardown, .disabled = 0);

Test(smoke, basic) { cr_assert(1, "hello"); }

Test(smoke, siege) { cr_assert(1, "hello"); }
