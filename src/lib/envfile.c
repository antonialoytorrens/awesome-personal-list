/* awesome-personal-list - optional KEY=VALUE config file, loaded into the environment. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "envfile.h"

void
load_env_file(const char *path)
{
	FILE	*fp;
	char	 line[2048];
	char	*eq;

	if ((fp = fopen(path, "r")) == NULL)
		return;		/* optional: no file is not an error */

	while (fgets(line, sizeof(line), fp) != NULL) {
		line[strcspn(line, "\r\n")] = '\0';

		if (line[0] == '\0' || line[0] == '#')
			continue;

		if ((eq = strchr(line, '=')) == NULL || eq == line)
			continue;

		*eq = '\0';
		setenv(line, eq + 1, 0);	/* 0: don't clobber a real env var */
	}

	fclose(fp);
}
