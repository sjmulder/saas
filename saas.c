#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#define USAGE		"usage: saas program [arguments ...]"
#define SOCKPATH	"/tmp/saas.sock"
#define MAXBACKLOG	64

static sig_atomic_t	killed;

static void
onsigint(int sig)
{
	(void) sig;

	killed = 1;
}

int
main(int argc, char **argv)
{
	int			fds, fdc;
	struct sigaction	sa;
	struct sockaddr_un	addr;

	(void) argv;

	if (argc < 2) {
		fputs(USAGE "\n", stderr);
		return 1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = onsigint;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);

	if ((fds = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		return 0;
	}

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKPATH, sizeof(addr.sun_path)-1);

	if (bind(fds, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("bind()");
		close(fds);
		goto cleanup_fds;
	}
	
	if (listen(fds, MAXBACKLOG) == -1) {
		perror("listen()");
		goto cleanup_bound;
	}
	
	printf("listening on %s\n", addr.sun_path);

	while ((fdc = accept(fds, NULL, NULL)) != -1) {
		puts("accepted");

		switch (fork()) {
		case -1:
			perror("fork()");
			goto cleanup_bound;
		case 0:
			close(fds);
			dup2(fdc, STDOUT_FILENO);
			dup2(fdc, STDERR_FILENO);
			close(fdc);
			close(STDIN_FILENO);
			execvp(argv[1], argv+1);
			perror("excecvp()");
			return 1;
		default:
			close(fdc);
			break;
		}
	}

	if (errno != EINTR)
		perror("accept()");

cleanup_bound:
	unlink(addr.sun_path);
cleanup_fds:
	close(fds);

	if (killed)
		raise(SIGINT);

	return 1;
}
