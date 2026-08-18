/* awesome-personal-list - optional KEY=VALUE config file, loaded into the environment. */

#ifndef AWESOME_PERSONAL_LIST_ENVFILE_H
#define AWESOME_PERSONAL_LIST_ENVFILE_H

/*
 * Reads `path` as lines of KEY=VALUE (blank lines and #-comments skipped)
 * and setenv()s each, without overwriting a variable already set. A
 * missing file is not an error -- the config it would provide is optional,
 * not required.
 */
void	load_env_file(const char *path);

#endif /* !AWESOME_PERSONAL_LIST_ENVFILE_H */
