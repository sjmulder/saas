#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/wait.h>
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
sigchld(int sig)
{
	(void)sig;

	while (waitpid(-1, NULL, WNOHANG) < 1)
		;
}

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

static void
listenany(char *host, char *port, fd_set *fds, int *fdmax)
{
	struct addrinfo		 hints;
	struct addrinfo		*ai0, *ai;
	int			 fd;
	struct sockaddr_storage	 addr;
	socklen_t		 addrlen;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_CANONNAME;

	if (getaddrinfo(host, port, &hints, &ai0) == -1)
		err(1, "getaddrinfo(%s, %s)", host, port);

	*fdmax = 0;
	FD_ZERO(fds);

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
		if (getsockname(fd, (struct sockaddr *)&addr, &addrlen) == -1)
			err(1, "getsockname()");

		FD_SET(fd, fds);
		if (fd > *fdmax)
			*fdmax = fd;

		printf("listening on %s\n", addrstr((struct sockaddr *)&addr,
		    addrlen));
	}

	if (!*fdmax)
		errx(1, "no suitable addresses");

	freeaddrinfo(ai0);
}

int
main(int argc, char **argv)
{
	char		 *host;
	char		 *port;
	char		**command;
	fd_set		  fds, readfds;
	int		  fd, fdmax, clientfd;
	struct sockaddr	  addr;
	socklen_t 	  addrlen;

	(void) argc;

	signal(SIGCHLD, sigchld);

	parseargs(argv, &host, &port, &command);
	listenany(host, port, &fds, &fdmax);

	while (1) {
		FD_COPY(&fds, &readfds);

		if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			err(1, "select()");
		}

		for (fd = 0; fd <= fdmax; fd++) {
			if (!FD_ISSET(fd, &readfds))
				continue;

			addrlen = sizeof(addr);
			if ((clientfd = accept(fd, &addr, &addrlen)) == -1)
				err(1, "accept()");

			printf("accepted %s\n", addrstr(&addr, addrlen));

			switch (fork()) {
			case -1:
				err(1, "fork()");
			case 0:
				for (fd = 0; fd <= fdmax; fd++)
					if (FD_ISSET(fd, &fds))
						close(fd);
				dup2(clientfd, STDOUT_FILENO);
				dup2(clientfd, STDERR_FILENO);
				dup2(clientfd, STDIN_FILENO);
				close(clientfd);
				execvp(*command, command);
				perror("excecvp()");
				return 1;
			default:
				close(clientfd);
				break;
			}
		}
	}

	return -1;
}
