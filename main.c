#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>

/*define sizes*/
#define WORD_SIZE (32) /*A word in the dictionary cannot surpass this limit of characters!*/
/*define errors*/
#define WRONG_ARGUMENTS (1)
#define CANNOT_OPEN_INPUT_FILE (2)
#define CANNOT_OPEN_DICTIONARY_FILE (3)
#define MALLOC_ERROR (4)
#define CANNOT_CLOSE_DICTIONARY_FILE (5)
#define CANNOT_SEEK_INPUT_FILE (6)
#define CANNOT_CLOSE_INPUT_FILE (7)
#define MEMCPY_ERROR (8)

/* structs */
struct search_word {
	char* definition;
	char* replacement;
	struct search_word* next;
};

/* typedefs */
typedef struct search_word search_word;

int main(int argc, char* argv[])
{
	int error_code;
	FILE* fp_in = NULL;
	FILE* fp_dict = NULL;

	/* Argument count check */
	if (argc != 3)
	{
		printf("Usage: limdict <input file> <dictionary file>\n");
		return WRONG_ARGUMENTS;
	}

	/* Open files, opened in textual mode to prevent EOL problems on Windows machines using CR LF*/
	error_code = fopen_s(&fp_in, argv[1], "rt");
	if (error_code != 0)
	{
		printf("Could not open %s\n", argv[1]);
		return CANNOT_OPEN_INPUT_FILE;
	}

	error_code = fopen_s(&fp_dict, argv[2], "rt");
	if (error_code != 0)
	{
		printf("Could not open %s\n", argv[2]);
		return CANNOT_OPEN_DICTIONARY_FILE;
	}

	/* Initiate dictionary building */
	char* dp = NULL;
	char* rp = NULL;
	int c; /* Not char to prevent EOF checking problems */
	int i;
	search_word* dict = NULL;
	search_word* dict_pointer = NULL;

	/* Make memory for dict item struct */
	dict_pointer = (search_word*)malloc(sizeof(search_word));
	if (dict_pointer == NULL)
	{
		printf("Malloc error\n");
		return MALLOC_ERROR;
	}

	/* Reference for the first item in dict */
	dict = dict_pointer;

	do {
		/* Make memory for dict item */
		dict_pointer->next = (search_word*)malloc(sizeof(search_word));
		if (dict_pointer->next == NULL)
		{
			printf("Malloc error\n");
			return MALLOC_ERROR;
		}

		/* Move pointer to next item*/
		dict_pointer = dict_pointer->next;

		/* Make memory for new definition and replacement */
		dp = (char*)malloc(WORD_SIZE);
		if (dp == NULL)
		{
			printf("Malloc error\n");
			return MALLOC_ERROR;
		}

		rp = (char*)malloc(WORD_SIZE);
		if (rp == NULL)
		{
			printf("Malloc error\n");
			return MALLOC_ERROR;
		}

		/* Copying dictionary values into memory */
		i = 0;
		while (isalpha(c = fgetc(fp_dict)))
		{
			dp[i] = c;
			i += 1;
		}
		dp[i] = '\0';

		i = 0;
		while (isalpha(c = fgetc(fp_dict)))
		{
			rp[i] = c;
			i += 1;
		}
		rp[i] = '\0';

		/* Add key and value to dict */
		dict_pointer->definition = dp;
		dict_pointer->replacement = rp;
		dict_pointer->next = NULL;

	} while (c != EOF);

	/* Free first unused malloc */
	dict_pointer = dict;
	dict = dict->next;
	free(dict_pointer);
	dict_pointer = NULL;

	/* Close fp_dict */
	error_code = fclose(fp_dict);
	if (error_code != 0)
	{
		printf("Could not close %s\n", argv[2]);
		return CANNOT_OPEN_DICTIONARY_FILE;
	}

	/* Initiate input buffer building */
	long long input_size;
	long long buffer_size;
	char* inp_buffer = NULL;
	char* found_word = NULL;
	int j;
	int diff = 0;

	/* Make memory for a single word buffer */
	found_word = (char*)malloc(WORD_SIZE);
	if (found_word == NULL)
	{
		printf("Malloc error\n");
		return MALLOC_ERROR;
	}

	/* Find input file size */
	error_code = fseek(fp_in, 0, SEEK_END);
	if (error_code != 0)
	{
		printf("Could not seek %s\n", argv[1]);
		return CANNOT_SEEK_INPUT_FILE;
	}

	input_size = ftell(fp_in);

	/* Round buffer_size to a power of 2 bigger than input_size */
	buffer_size = 1;
	while (buffer_size < input_size)
	{
		buffer_size *= 2;
	}

	/* Make memory for input file content */
	inp_buffer = (char*)malloc(buffer_size);
	if (inp_buffer == NULL)
	{
		printf("Malloc error\n");
		return MALLOC_ERROR;
	}

	/* Seek back to the start of the file */
	error_code = fseek(fp_in, 0, SEEK_SET);
	if (error_code != 0)
	{
		printf("Could not seek %s\n", argv[1]);
		return CANNOT_SEEK_INPUT_FILE;
	}

	/* Read input to memory */
	fread_s(inp_buffer, buffer_size, sizeof(char), input_size, fp_in);

	/* Process input buffer */
	for (i = 0; i < input_size; ++i)
	{
		if (isalpha(inp_buffer[i]))
		{
			j = i+1;
			while (isalpha(inp_buffer[j]))
			{
				++j;
			}

			/* Put word in found_word buffer */
			found_word = (char*)memcpy(found_word, inp_buffer + i, j - i);
			if (found_word == NULL)
			{
				printf("Memcpy error\n");
				return MEMCPY_ERROR;
			}

			/* Add null terminator */
			found_word[j - i] = '\0';

			/* Search word in dict */
			dict_pointer = dict;
			while (dict_pointer != NULL)
			{
				/* Compare each definition in dict to found_word */
				if (strcmp(dict_pointer->definition, found_word) == 0)
				{
					/* Make space for replacement */
					diff = (j - i) - strlen(dict_pointer->replacement);
					if (diff != 0)
					{
						error_code = memcpy(inp_buffer + j - diff, inp_buffer + j, input_size - j);
						if (error_code == NULL) {
							printf("Memcpy error\n");
							return MEMCPY_ERROR;
						}
					}

					/* Replace found_word with replacement */
					error_code = memcpy(inp_buffer + i, dict_pointer->replacement, strlen(dict_pointer->replacement));
					if (error_code == NULL) {
						printf("Memcpy error\n");
						return MEMCPY_ERROR;
					}

					/* Break out of dict search */
					break;
				}
				/* Increment dict pointer */
				dict_pointer = dict_pointer->next;
			}

			/* Skip processed letters and update input_size */
			i = j - diff;
			input_size -= diff;

			/* Reset diff */
			diff = 0;
		}
	}

	/* Free found_word malloc */
	free(found_word);

	/* Free dict memory */
	while (dict != NULL)
	{
		dict_pointer = dict->next;
		free(dict);
		dict = dict_pointer;
	}

	/* Close fp_in to open again in w mode (deletes content) */
	error_code = fclose(fp_in);
	if (error_code != 0)
	{
		printf("Could not close %s\n", argv[1]);
		return CANNOT_CLOSE_INPUT_FILE;
	}

	/* Open fp_in in w mode */
	error_code = fopen_s(&fp_in, argv[1], "wt");
	if (error_code != 0)
	{
		printf("Could not open %s\n", argv[1]);
		return CANNOT_OPEN_INPUT_FILE;
	}

	/* Save inp_buffer to fp_in */
	fwrite(inp_buffer, sizeof(char), input_size, fp_in);

	/* Close fp_in */
	error_code = fclose(fp_in);
	if (error_code != 0)
	{
		printf("Could not close %s\n", argv[1]);
		return CANNOT_CLOSE_INPUT_FILE;
	}

	/* Free inp_buffer */
	free(inp_buffer);

	return 0;
}