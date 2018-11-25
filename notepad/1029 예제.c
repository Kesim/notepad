#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

main() {

	printf("--> Before exec function\n");

	if (execlp("ls", "ls", "-a", (char *)NULL) == -1) {
		perror("execlp");
		exit(1);
	}
	printf("--> After exec function\n");

}
============================
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

main() {
	char *argv[3];

	printf("--> Before exec function\n");

	argv[0] = "ls";
	argv[1] = "-a";
	argv[2] = NULL;
	if (execv("/bin/ls", argv) == -1) {
		perror("execv");
		exit(1);
	}

	printf("--> After exec function\n");

}
===========================
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

main() {
	char *argv[3];
	char *envp[2];

	printf("--> Before exec function\n");

	argv[0] = "arg.out";
	argv[1] = "100";
	argv[2] = NULL;

	envp[0] = "MYENV=hanbit";
	envp[1] = NULL;

	if (execve("./arg.out", argv, envp) == -1) {
		perror("execve");
		exit(1);
	}

	printf("--> After exec function\n");
}