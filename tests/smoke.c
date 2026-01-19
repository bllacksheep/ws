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
static pid_t siege_pid;

void setup(void) {
  char *path = realpath(SERVERBIN, NULL);
  cr_assert(path != NULL, "error: realpath");

  pid_t pid = fork();
  cr_assert(pid != -1, "error: fork");

  if (pid == 0) {
    int rc = execl(path, "server", ADDR, PORT, (char *)NULL);
    if (rc <0) {
        perror("error: server exec");
        exit(-1);
    }
  } else {
    server_pid = pid;
    sleep(1);
  }
}

void teardown(void) {
  pid_t pids[2] = {
      server_pid,
      siege_pid,
  };

  size_t num_pids = sizeof(pids) / sizeof(pids[0]);
  for (int i = 0; i < num_pids; i++) {
    kill(pids[i], SIGTERM);
    int status;
    waitpid(pids[i], &status, 0);
    if (WIFEXITED(status)) {
      printf("exit status: %d", WEXITSTATUS(status));
    }
  }
}

TestSuite(smoke, .init = setup, .fini = teardown, .disabled = 0);

Test(smoke, basic) {
  pid_t pid = fork();
  if (pid == 0) {
    char address[100];
    sprintf(address, "http://%s:%s/chat", ADDR, PORT);
    int rc = execl("/usr/bin/siege", "siege", "-c5", "-r2", address, (char *)NULL);
    if (rc <0) {
        perror("error: server exec");
        exit(-1);
    }
  } else {
    siege_pid = pid;
    sleep(1);
  }
}

// Test(smoke, siege) { cr_assert(1, "hello"); }
