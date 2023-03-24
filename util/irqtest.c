#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Send SIGINT signal to self\n");
    kill(getpid(), SIGINT);
    return 0;
}