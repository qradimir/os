#include <signal.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int caught = 0;
pid_t cpid;
int is_alarm = 0;

void handler(int sig, siginfo_t *info, void *e){
	if (sig == SIGALRM) {
		is_alarm = 1;
		return;
	}
	if (caught == 0) {
		caught = sig;
		cpid = info->si_pid;
	}
}

int main() {
	struct sigaction act;
	act.sa_sigaction = &handler;
	act.sa_flags = SA_SIGINFO;

	if (sigemptyset(&act.sa_mask)
			|| sigaddset(&act.sa_mask, SIGALRM)
			|| sigaddset(&act.sa_mask, SIGUSR1) 
			|| sigaddset(&act.sa_mask, SIGUSR2)
			|| sigaction(SIGALRM, &act, NULL) 
			|| sigaction(SIGUSR1, &act, NULL) 
			|| sigaction(SIGUSR2, &act, NULL) ) {
		fprintf(stderr, "sigaction init error: %s\n", strerror(errno));
		return 1;
	}

	alarm(10);
	while (is_alarm == 0) {
		if (caught != 0) {
			printf("%s from %d\n", caught == SIGUSR1 ? "SIGUSR1" : "SIGUSR2", cpid);
			return 0;
		}
		pause();
	}

	printf("No signals were caught\n");
	return 0;
}