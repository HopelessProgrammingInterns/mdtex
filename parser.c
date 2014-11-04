#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define PAGE_SIZE 512

typedef enum
{
	NONE,
	HEADER,

	BOLD,
	UNDERLINE,
	ITALIC,

	LIST_ITEM,
	UNORDERED_LIST_ITEM,

	HEADING_1,
	HEADING_2,
	HEADING_3,
	HEADING_4,
	HEADING_5,
	HEADING_6,

	LINK,
	CODE,
	MULTILINE_CODE,
	MATH
} Token;

typedef struct {
	Token type;
	char *data;
	int len;
} Element;

#define is_whitespace(c) ((c) == ' ' || (c) == '\t' || c == '\n')

char advance_char (char **c, int *col, int *row)
{
	(*c)++;

	if (**c == '\n') {
		(*row)++;
		col = 0;
	} else
		(*col)++;

	return **c;
}

int
parse (int fd, int len, const char *error)
{
	char *c;
	Token current_token;
	Element *list;
	Element *current_element;
	int is_start_of_line = 1;
	int row, col;

#define next_element() \
	current_element++; \
	current_element->data = c; \
	if (!current_element) \
		exit (1);

#define skip_whitespaces() \
	while (is_whitespace(*c)) advance_char (&c, &col, &row)

#define finish_none_element() \
	if (current_element->len > 0) { \
		current_element->len++; \
		next_element (); \
	}

#define next_char() advance_char (&c, &col, &row)

	list = (Element *) calloc (PAGE_SIZE, sizeof (Element));
	current_element = list;

	current_element->type = NONE;

	c = (char *) mmap (NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);

	if (c == MAP_FAILED)
		return 0;

	do {
		if (is_start_of_line)
			skip_whitespaces ();

		switch (*c) {
			case '{':
				finish_none_element ();

				if (!is_start_of_line)
					break;

				current_element->type = HEADER;
				current_element->data = c;

				// just assume header and skip for now
				while (next_char () != '}') {
					if (*c == '\0') {
						error = "Header not terminated";
						return 0;
					}

					current_element->len++;
				}
				current_element->len += 2;

				next_char ();
				next_element ();
				break;
			case '*':
				finish_none_element ();

				if (is_start_of_line) {
					current_element->type = LIST_ITEM;
					next_char ();
					skip_whitespaces ();
					next_element ();
				} else {
					current_element->type = BOLD;
					current_element->data = c + 1;

					while (next_char () != '*') {
						if (!*c) {
							error = "Bold not terminated";
							return 0;
						}

						current_element->len++;
					}

					next_char ();
					next_element ();
				}

				break;
			case '`':
				finish_none_element ();

				if (c[1] == '`' && c[2] == '`') {
					current_element->type = MULTILINE_CODE;
					next_char ();
					next_char ();

					current_element->data = c;
					while (!(*c == '`' && c[1] == '`' && c[2] == '`'))
						current_element->len++;
				} else {
					current_element->type = CODE;
					current_element->data = c + 1;

					while (next_char () != '`') {
						if (!*c) {
							error = "Code literal not terminated";
							return 0;
						}

						current_element->len++;
					}
				}

				next_element ();
				break;
			case '$':
				finish_none_element ();

				current_element->type = MATH;
				current_element->data = c + 1;

				while (next_char () != '$') {
					if (!*c) {
						error = "Math literal not terminated";
						return 0;
					}

					current_element->len++;
				}

				next_char ();

				next_element ();
				break;
			case '#':
				finish_none_element ();

				if (is_start_of_line) {
					current_element->type = HEADING_1;
					while (next_char () == '#')
						(*(int *) (&current_element->type))++;

					skip_whitespaces ();

					current_element->data = c;
					while (next_char () != '\n')
						current_element->len++;

					current_element->len++;

					next_element ();
				}
				break;
			case '_':
				finish_none_element ();

				current_element->type = ITALIC;
				current_element->data = c + 1;

				while (next_char () != '_') {
					if (!*c) {
						error = "Italic not terminated";
						return 0;
					}

					current_element->len++;
				}

				next_element ();
				break;
			default:
				if (current_element->type == NONE)
					current_element->len++;
				break;
		}

		is_start_of_line = *c == '\n';
	} while (next_char ());

	finish_none_element ();

	Element *i;
	for (i = list; i != current_element; i++) {
		printf ("%i, %.*s\n", i->type, i->len, i->data);
	}

	return 1;
}

int
main (int argc, char **argv)
{
	char *path;
	int fd;
	struct stat st;
	char error[512];
   
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

	if (!parse (fd, st.st_size, error)) {
		perror ("Parsing failed");
		exit (1);
	}

	return 0;
}
