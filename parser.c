#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef enum
{
	NONE,

	ASTERIK,
	UNDERSCORE,
	SHARP,
	EQUAL,

	BRACKET_OPEN,
	BRACKET_CLOSE,
	MATH
} Token;

int
parse (int fd, int len)
{
	char *c;
	Token current_token;

	c = (char *) mmap (NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);

	if (c == MAP_FAILED)
		return 0;

	do {
		// printf ("%c\n", *c);
		switch (*c) {
			case '*':
				break;
		}
	} while (*(++c));

	return 1;
}

int
main (int argc, char **argv)
{
	char *path;
	int fd;
	struct stat st;
   
	path = argv[1];

	if (!path)
		fprintf (stderr, "No file given!!!\n");

	fd = open (path, O_RDONLY);

	if (fd < 0) {
		fprintf (stderr, "open failed\n");
		exit (1);
	}

	if (fstat (fd, &st) < 0) {
		fprintf (stderr, "Stat failed\n");
		exit (1);
	}

	if (!parse (fd, st.st_size)) {
		perror ("Parsing failed");
		exit (1);
	}

	return 0;
}
