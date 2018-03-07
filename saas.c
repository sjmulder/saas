#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#define USAGE		"usage: saas [name [port]] -- program ..."
#define DEFSOCK		"/tmp/saas.sock"
#define MAXBACKLOG	64

static void
parseargs(char **argv, char **host, char **port, char ***command)
{
	int	i;
	int	seppos	= 0;

	for (i = 1; argv[i]; i++) {
		if (!strcmp(argv[i], "--")) {
			seppos = i;
			break;
		}
	}

	if (seppos > 3) {
		warnx("too many arguments");
		fputs(USAGE "\n", stderr);
		exit(1);
	}

	*host = seppos > 1 ? argv[1] : DEFSOCK;
	*port = seppos > 2 ? argv[2] : NULL;
	*command = &argv[seppos+1];

	if (!**command) {
		warnx("no command given");
		fputs(USAGE "\n", stderr);
		exit(1);
	}
}

static char *
addrstr(struct sockaddr *addr, socklen_t addrlen)
{
	static char *s;

	char	name[NI_MAXHOST];
	char	serv[NI_MAXSERV];
	int	res;

	res = getnameinfo(addr, addrlen, name, sizeof(name), serv,
	    sizeof(serv), NI_NUMERICHOST);
	if (res == -1)
		err(1, "getnameinfo()");

	if (s) {
		free(s);
		s = NULL;
	}

	if (!name[0] && !serv[0])
		return "anonymous socket";

	if (!serv[0])
		res = asprintf(&s, "%s", name);
	else if (addr->sa_family == AF_INET6)
		res = asprintf(&s, "[%s]:%s", name, serv);
	else
		res = asprintf(&s, "%s:%s", name, serv);

	if (res == -1)
		err(1, "asprintf()");

	return s;

}

static int
listenany(char *host, char *port)
{
	struct addrinfo	 hints;
	struct addrinfo	*ai0, *ai;
	int		 fd;
	struct sockaddr	 addr;
	socklen_t	 addrlen;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_CANONNAME;

	if (getaddrinfo(host, port, &hints, &ai0) == -1)
		err(1, "gettadrinfo(%s, %s)", host, port);

	for (ai = ai0; ai; ai = ai->ai_next) {
		printf("binding to %s... ", addrstr(ai->ai_addr,
		    ai->ai_addrlen));

		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd == -1) {
			printf("%s\n", strerror(errno));
			continue;
		}

		if (ai->ai_family == AF_UNIX)
			unlink(host);

		if (bind(fd, ai->ai_addr, ai->ai_addrlen) == -1 ||
		    listen(fd, MAXBACKLOG) == -1) {
			printf("%s\n", strerror(errno));
			close(fd);
			continue;
		}

		addrlen = sizeof(addr);
		if (getsockname(fd, &addr, &addrlen) == -1)
			err(1, "getsockname()");

		printf("success\nlistening on %s\n", addrstr(&addr, addrlen));
		return fd;
	}

	errx(1, "no suitable addresses");
}

int
main(int argc, char **argv)
{
	char		 *host;
	char		 *port;
	char		**command;
	int		  fds, fdc;
	struct sockaddr	  addr;
	socklen_t 	  addrlen;

	(void) argc;

	parseargs(argv, &host, &port, &command);
	fds = listenany(host, port);

	while (1) {
		addrlen = sizeof(addr);
		if ((fdc = accept(fds, &addr, &addrlen)) == -1)
			err(1, "accept()");

		printf("accepted %s\n", addrstr(&addr, addrlen));

		switch (fork()) {
		case -1:
			err(1, "fork()");
		case 0:
			close(fds);
			dup2(fdc, STDOUT_FILENO);
			dup2(fdc, STDERR_FILENO);
			close(fdc);
			close(STDIN_FILENO);
			execvp(*command, command);
			perror("excecvp()");
			return 1;
		default:
			close(fdc);
			break;
		}
	}

	return -1;
}
