/*
 * Program name: EECS 348 Assignment 2 - Email Priority Queue
 * Description: This C program reads email records and commands, stores the
 *              emails in a max-heap, and processes the highest-priority email.
 * Inputs:      Standard input or an optional input-file command-line argument.
 *              Input commands are EMAIL, NEXT, READ, and COUNT.
 * Outputs:     Terminal messages showing the next email, email counts, and
 *              warnings or errors for invalid input.
 * Collaborators and Other sources for code: Claude generated the latex pdf to make it smaller for the code comparison, used gemini to assist on the code.
 * Author:      Heet Jani
 * Creation date: 2026-09-17
 * Revision date: 2026-09-17
 * Revisions:  Added the required prologue and explanatory source comments.
 */

/* Provides input/output functions such as printf, fprintf, fopen, and fgets. */
#include <stdio.h> /* Include standard input and output declarations. */
/* Provides dynamic-memory functions such as realloc and free. */
#include <stdlib.h> /* Include memory-management and conversion declarations. */
/* Provides string-processing functions such as strcmp, strchr, and strlen. */
#include <string.h> /* Include string-processing declarations. */
/* Provides character classification through the isspace function. */
#include <ctype.h> /* Include character-classification declarations. */

/* Sets the maximum number of characters read from one input line. */
#define MAX_LINE    1024 /* Set the maximum input-line length. */
/* Sets the maximum stored length of an email subject, including its terminator. */
#define MAX_SUBJECT 512 /* Set the maximum email-subject length. */
/* Sets the maximum stored length of a sender name, including its terminator. */
#define MAX_SENDER  64 /* Set the maximum sender-name length. */
/* Sets the maximum stored length of a date string, including its terminator. */
#define MAX_DATE    16 /* Set the maximum date-text length. */
/* Records the number of recognized sender-priority categories. */
#define NUM_CATEGORIES 5 /* Set the number of sender categories. */

/* Maps each recognized sender category to its priority rank. */
static const char *CATEGORY_NAMES[NUM_CATEGORIES] = { /* Begin the sender-category name table. */
    "Boss",             /* Highest-priority sender category. */
    "Subordinate",      /* Second-priority sender category. */
    "Peer",             /* Third-priority sender category. */
    "ImportantPerson",  /* Fourth-priority sender category. */
    "OtherPerson"       /* Lowest-priority sender category. */
}; /* End the sender-category name table. */

/* Stores the numeric components of an email date. */
typedef struct { /* Begin the date structure definition. */
    int month;           /* Stores the month component. */
    int day;             /* Stores the day component. */
    int year;            /* Stores the year component. */
} Date; /* End the date structure definition. */

/* Stores all information needed to rank and display one email. */
typedef struct { /* Begin the email structure definition. */
    char sender[MAX_SENDER];    /* Stores the sender category text. */
    char subject[MAX_SUBJECT];  /* Stores the email subject text. */
    char date_text[MAX_DATE];   /* Stores the original date representation. */
    Date date;                  /* Stores the parsed date components. */
    int  rank;                  /* Stores the sender category priority. */
    long sequence;              /* Stores arrival order for tie-breaking. */
} Email; /* End the email structure definition. */

/* Stores a dynamically sized array that represents the email max-heap. */
typedef struct { /* Begin the max-heap structure definition. */
    Email *items;               /* Points to the allocated heap elements. */
    int    size;                /* Counts the elements currently in the heap. */
    int    capacity;            /* Records the allocated element capacity. */
} MaxHeap; /* End the max-heap structure definition. */

/* Author: Heet Jani. Compares two emails for max-heap priority. */
static int email_compare(const Email *a, const Email *b) /* Compare two emails and return their priority order. */
{ /* Begin comparing two email records. */
    if (a->rank != b->rank)                 /* Compare sender categories first. */
        return (a->rank < b->rank) ? 1 : -1; /* Lower category index has priority. */

    if (a->date.year != b->date.year)       /* Compare years when ranks match. */
        return (a->date.year > b->date.year) ? 1 : -1; /* Newer year wins. */
    if (a->date.month != b->date.month)     /* Compare months when years match. */
        return (a->date.month > b->date.month) ? 1 : -1; /* Newer month wins. */
    if (a->date.day != b->date.day)         /* Compare days when months match. */
        return (a->date.day > b->date.day) ? 1 : -1; /* Newer day wins. */

    if (a->sequence != b->sequence)         /* Compare arrival order last. */
        return (a->sequence > b->sequence) ? 1 : -1; /* Later arrival wins. */

    return 0;                               /* Equal records have equal priority. */
} /* End comparing two email records. */

/* Author: Heet Jani. Initializes an empty heap with no allocated storage. */
static void heap_init(MaxHeap *h) /* Initialize the heap storage and counters. */
{ /* Begin initializing the heap. */
    h->items    = NULL;                      /* No element array exists yet. */
    h->size     = 0;                         /* The heap starts empty. */
    h->capacity = 0;                         /* No storage is currently reserved. */
} /* End initializing the heap. */

/* Author: Heet Jani. Releases heap storage and resets its bookkeeping. */
static void heap_free(MaxHeap *h) /* Release heap storage and reset its counters. */
{ /* Begin releasing heap resources. */
    free(h->items);                          /* Return the dynamic array to the OS. */
    h->items    = NULL;                      /* Prevent use of the released pointer. */
    h->size     = 0;                         /* Reset the element count. */
    h->capacity = 0;                         /* Reset the allocated capacity. */
} /* End releasing heap resources. */

/* Author: Heet Jani. Reports whether the heap contains no emails. */
static int heap_is_empty(const MaxHeap *h) /* Return whether the heap currently contains zero emails. */
{ /* Begin checking whether the heap is empty. */
    return h->size == 0;                      /* Zero elements means the heap is empty. */
} /* End checking whether the heap is empty. */

/* Author: Heet Jani. Returns the number of emails currently stored. */
static int heap_count(const MaxHeap *h) /* Return the number of emails currently stored in the heap. */
{ /* Begin counting heap elements. */
    return h->size;                           /* The size field is the email count. */
} /* End counting heap elements. */

/* Author: Heet Jani. Expands the heap array until it can hold needed elements. */
static int heap_reserve(MaxHeap *h, int needed) /* Ensure the heap has room for the requested number of emails. */
{ /* Begin reserving heap storage. */
    int new_capacity;                         /* Stores the next allocation capacity. */
    Email *grown;                             /* Receives the resized allocation. */

    if (needed <= h->capacity)                /* Existing storage is already sufficient. */
        return 1;                             /* Report successful reservation. */

    new_capacity = (h->capacity == 0) ? 8 : h->capacity * 2; /* Choose initial or doubled size. */
    while (new_capacity < needed)              /* Continue doubling until enough space exists. */
        new_capacity *= 2;                     /* Grow geometrically for efficient inserts. */

    grown = (Email *)realloc(h->items, (size_t)new_capacity * sizeof(Email)); /* Resize storage. */
    if (grown == NULL)                         /* Detect allocation failure. */
        return 0;                              /* Tell the caller that growth failed. */

    h->items    = grown;                       /* Store the resized array pointer. */
    h->capacity = new_capacity;                /* Record its new capacity. */
    return 1;                                  /* Report successful reservation. */
} /* End reserving heap storage. */

/* Author: Heet Jani. Exchanges two email elements in the heap array. */
static void heap_swap(MaxHeap *h, int i, int j) /* Exchange two heap elements. */
{ /* Begin exchanging two heap elements. */
    Email temp = h->items[i];                  /* Save the first element temporarily. */
    h->items[i] = h->items[j];                 /* Move the second element into the first slot. */
    h->items[j] = temp;                        /* Move the saved element into the second slot. */
} /* End exchanging two heap elements. */

/* Author: Heet Jani. Moves an inserted element toward the heap root. */
static void sift_up(MaxHeap *h, int index) /* Move one element upward to restore heap order. */
{ /* Begin moving an element toward the heap root. */
    while (index > 0) {                        /* Begin moving upward until the root is reached. */
        int parent = (index - 1) / 2;          /* Calculate the current element's parent. */
        if (email_compare(&h->items[index], &h->items[parent]) > 0) { /* Check heap order. */
            heap_swap(h, index, parent);       /* Move the higher-priority email upward. */
            index = parent;                    /* Continue checking from the parent position. */
        } else {                               /* Begin the branch where heap order is correct. */
            break;                             /* Stop when the heap order is correct. */
        }                                        /* End the heap-order decision. */
    }                                            /* End the upward movement loop. */
}                                                /* End moving an element upward. */

/* Author: Heet Jani. Moves a replacement element down from the heap root. */
static void sift_down(MaxHeap *h, int index) /* Move one element downward to restore heap order. */
{ /* Begin moving an element toward the heap leaves. */
    for (;;) {                                 /* Begin repeating until the replacement is positioned. */
        int left    = 2 * index + 1;           /* Calculate the left child index. */
        int right   = 2 * index + 2;           /* Calculate the right child index. */
        int largest = index;                   /* Assume the current element is largest. */

        if (left < h->size &&                 /* Ensure the left child exists. */
            email_compare(&h->items[left], &h->items[largest]) > 0) /* Compare left child. */
            largest = left;                   /* Select left child when it has higher priority. */

        if (right < h->size &&                /* Ensure the right child exists. */
            email_compare(&h->items[right], &h->items[largest]) > 0) /* Compare right child. */
            largest = right;                  /* Select right child when it is highest priority. */

        if (largest == index)                 /* Check whether the heap order is restored. */
            break;                            /* Stop when no child should move upward. */

        heap_swap(h, index, largest);          /* Exchange the element with its best child. */
        index = largest;                       /* Continue from the child's former position. */
    }                                           /* End the downward movement loop. */
}                                               /* End moving an element downward. */

/* Author: Heet Jani. Inserts an email and restores max-heap order. */
static int heap_insert(MaxHeap *h, const Email *e) /* Insert one email and return whether insertion succeeded. */
{ /* Begin inserting an email into the heap. */
    if (!heap_reserve(h, h->size + 1))       /* Reserve space for the new email. */
        return 0;                            /* Report failure if allocation failed. */

    h->items[h->size] = *e;                  /* Place the email after the current last item. */
    h->size++;                               /* Increase the number of stored emails. */
    sift_up(h, h->size - 1);                 /* Restore max-heap order from the new item. */
    return 1;                                /* Report a successful insertion. */
} /* End inserting an email into the heap. */

/* Author: Heet Jani. Copies the highest-priority email without removing it. */
static int heap_peek(const MaxHeap *h, Email *out) /* Copy the highest-priority email without removing it. */
{ /* Begin inspecting the highest-priority email. */
    if (heap_is_empty(h))                     /* Check whether a root email exists. */
        return 0;                             /* Report that no email can be returned. */

    *out = h->items[0];                        /* Copy the root, highest-priority email. */
    return 1;                                  /* Report a successful peek operation. */
} /* End inspecting the highest-priority email. */

/* Author: Heet Jani. Removes the highest-priority email from the heap. */
static int heap_remove_max(MaxHeap *h, Email *out) /* Remove and optionally copy the highest-priority email. */
{ /* Begin removing the highest-priority email. */
    if (heap_is_empty(h))                      /* Check whether there is an email to remove. */
        return 0;                              /* Report that removal was not possible. */

    if (out != NULL)                           /* Check whether the caller requested the email. */
        *out = h->items[0];                    /* Copy the root before changing the heap. */

    h->items[0] = h->items[h->size - 1];       /* Move the final item into the root position. */
    h->size--;                                 /* Reduce the number of stored emails. */

    if (h->size > 0)                           /* Sift only when another item remains. */
        sift_down(h, 0);                       /* Restore max-heap order from the root. */

    return 1;                                  /* Report a successful removal. */
} /* End removing the highest-priority email. */

/* Author: Heet Jani. Removes leading and trailing whitespace in place. */
static char *trim(char *s) /* Remove leading and trailing whitespace from a string. */
{ /* Begin trimming whitespace from a string. */
    char *end;                                 /* Points to the final non-whitespace search area. */

    while (*s != '\0' && isspace((unsigned char)*s)) /* Skip leading whitespace. */
        s++;                                   /* Advance to the first useful character. */

    if (*s == '\0')                            /* Check whether only whitespace was present. */
        return s;                               /* Return the string's terminating position. */

    end = s + strlen(s) - 1;                   /* Point to the current final character. */
    while (end > s && isspace((unsigned char)*end)) /* Skip trailing whitespace. */
        end--;                                 /* Move backward toward the string start. */
    end[1] = '\0';                             /* Terminate immediately after the final text. */

    return s;                                  /* Return the trimmed string's first character. */
} /* End trimming whitespace from a string. */

/* Author: Heet Jani. Copies text while guaranteeing a null terminator. */
static void copy_bounded(char *dest, size_t dest_size, const char *src) /* Copy text within the destination bound. */
{ /* Begin copying a bounded string. */
    size_t i;                                  /* Tracks the destination position being copied. */
    for (i = 0; i + 1 < dest_size && src[i] != '\0'; i++) /* Respect buffer boundaries. */
        dest[i] = src[i];                      /* Copy one source character. */
    dest[i] = '\0';                            /* Add the required string terminator. */
} /* End copying a bounded string. */

/* Author: Heet Jani. Converts a sender category name into its priority index. */
static int category_rank(const char *name) /* Find and return the priority rank for a sender category. */
{ /* Begin finding a sender category rank. */
    int i;                                     /* Tracks the category currently being checked. */
    for (i = 0; i < NUM_CATEGORIES; i++) {     /* Begin visiting each recognized category. */
        if (strcmp(name, CATEGORY_NAMES[i]) == 0) /* Compare the input name with this category. */
            return i;                          /* Return the matching category's rank. */
    }                                          /* End visiting the category table. */
    return -1;                                /* Report that the category was not recognized. */
}                                              /* End finding a sender category rank. */

/* Author: Heet Jani. Parses and validates a date in month-day-year form. */
static int parse_date(const char *text, Date *out) /* Parse a date string and return whether it is valid. */
{ /* Begin parsing and validating a date. */
    int month = 0, day = 0, year = 0;          /* Receive the three date components. */
    char extra;                                /* Detect unexpected characters after the date. */

    if (sscanf(text, "%d-%d-%d%c", &month, &day, &year, &extra) != 3) /* Parse exact form. */
        return 0;                              /* Reject malformed date text. */

    if (month < 1 || month > 12 || day < 1 || day > 31 || year < 0) /* Validate ranges. */
        return 0;                              /* Reject impossible component values. */

    out->month = month;                        /* Save the validated month. */
    out->day   = day;                          /* Save the validated day. */
    out->year  = year;                         /* Save the validated year. */
    return 1;                                  /* Report successful parsing. */
} /* End parsing and validating a date. */

/* Author: Heet Jani. Parses one email record from comma-separated text. */
static int parse_email(char *body, long sequence, Email *out) /* Parse an email record and return whether it is valid. */
{ /* Begin parsing and validating an email record. */
    char *first_comma, *second_comma;         /* Locate the two field separators. */
    char *sender, *subject, *date_text;       /* Point to the three parsed fields. */
    int rank;                                  /* Stores the sender's priority rank. */

    first_comma = strchr(body, ',');            /* Find the separator after the sender. */
    if (first_comma == NULL)                   /* Reject an email without that separator. */
        return 0;                              /* Report malformed email data. */
    *first_comma = '\0';                       /* End the sender field at the comma. */

    second_comma = strchr(first_comma + 1, ','); /* Find the separator after the subject. */
    if (second_comma == NULL)                  /* Reject an email without the second separator. */
        return 0;                              /* Report malformed email data. */
    *second_comma = '\0';                      /* End the subject field at the comma. */

    sender    = trim(body);                    /* Remove whitespace from the sender field. */
    subject   = trim(first_comma + 1);         /* Remove whitespace from the subject field. */
    date_text = trim(second_comma + 1);        /* Remove whitespace from the date field. */

    rank = category_rank(sender);              /* Convert the sender name into a rank. */
    if (rank < 0)                              /* Check whether the sender was recognized. */
        return 0;                              /* Reject unknown sender categories. */

    if (!parse_date(date_text, &out->date))    /* Parse and validate the email date. */
        return 0;                              /* Reject malformed dates. */

    copy_bounded(out->sender,    sizeof(out->sender),    sender);    /* Store sender safely. */
    copy_bounded(out->subject,   sizeof(out->subject),   subject);   /* Store subject safely. */
    copy_bounded(out->date_text, sizeof(out->date_text), date_text); /* Store date text safely. */
    out->rank     = rank;                       /* Store the calculated priority rank. */
    out->sequence = sequence;                   /* Store arrival order for tie-breaking. */

    return 1;                                  /* Report a successfully parsed email. */
} /* End parsing and validating an email record. */

/* Author: Heet Jani. Displays the highest-priority email without removing it. */
static void do_next(const MaxHeap *h) /* Display the highest-priority unread email. */
{ /* Begin displaying the next email. */
    Email top;                                 /* Receives a copy of the heap's root email. */

    if (!heap_peek(h, &top)) {                  /* Begin handling an empty heap. */
        printf("No emails to read.\n");       /* Inform the user when the heap is empty. */
        return;                                /* Stop this command without further output. */
    }                                          /* End handling an empty heap. */

    printf("Next email:\n");                   /* Label the email being displayed. */
    printf("Sender: %s\n", top.sender);       /* Print the sender category. */
    printf("Subject: %s\n", top.subject);     /* Print the email subject. */
    printf("Date: %s\n", top.date_text);      /* Print the original date text. */
} /* End displaying the next email. */

/* Author: Heet Jani. Removes the highest-priority email for a READ command. */
static void do_read(MaxHeap *h) /* Remove the highest-priority unread email. */
{ /* Begin processing a READ command. */
    heap_remove_max(h, NULL);                  /* Remove the root and discard its details. */
} /* End processing a READ command. */

/* Author: Heet Jani. Prints the number of unread emails. */
static void do_count(const MaxHeap *h) /* Display the number of unread emails. */
{ /* Begin processing a COUNT command. */
    printf("There are %d emails to read.\n", heap_count(h)); /* Display the heap size. */
} /* End processing a COUNT command. */

/* Author: Heet Jani. Reads and executes every command from an input stream. */
static void process_stream(FILE *in, MaxHeap *heap) /* Read and execute commands from the input stream. */
{ /* Begin reading and processing the input stream. */
    char line[MAX_LINE];                       /* Stores one input line at a time. */
    long line_number = 0;                      /* Tracks source line numbers for warnings. */
    long arrival     = 0;                      /* Tracks valid email arrival order. */

    while (fgets(line, sizeof(line), in) != NULL) { /* Begin processing each input line. */
        char *text;                             /* Points to the trimmed input line. */
        line_number++;                          /* Count the line just read. */

        text = trim(line);                      /* Remove surrounding whitespace. */
        if (*text == '\0')                      /* Ignore blank input lines. */
            continue;                           /* Move to the next source line. */

        if (strncmp(text, "EMAIL", 5) == 0 && /* Check for the EMAIL command prefix. */
            (text[5] == '\0' || isspace((unsigned char)text[5]))) { /* Require a boundary. */
            Email e;                            /* Receives the parsed email record. */
            char *body = trim(text + 5);        /* Remove the command prefix and whitespace. */

            if (parse_email(body, arrival, &e)) { /* Begin handling a valid email body. */
                arrival++;                       /* Advance arrival order for the valid email. */
                if (!heap_insert(heap, &e)) {    /* Begin handling an insertion failure. */
                    fprintf(stderr,              /* Report allocation failure to standard error. */
                            "Error: out of memory while storing email on line %ld.\n", /* Format the memory-error message. */
                            line_number);        /* Include the failing source line number. */
                    return;                      /* Stop processing after an unrecoverable error. */
                }                               /* End handling an insertion failure. */
            } else {                            /* Begin handling malformed email data. */
                fprintf(stderr,                   /* Report malformed email input. */
                        "Warning: skipping malformed email on line %ld.\n", /* Format the malformed-email warning. */
                        line_number);             /* Include the malformed source line number. */
            }                                   /* End handling malformed email data. */
        } else if (strcmp(text, "NEXT") == 0) {  /* Check for the display command. */
            do_next(heap);                       /* Display the current highest-priority email. */
        } else if (strcmp(text, "READ") == 0) {  /* Check for the removal command. */
            do_read(heap);                       /* Remove the current highest-priority email. */
        } else if (strcmp(text, "COUNT") == 0) { /* Check for the count command. */
            do_count(heap);                      /* Display the number of stored emails. */
        } else {                                /* Begin handling an unrecognized command. */
            fprintf(stderr,                      /* Report commands that are not recognized. */
                    "Warning: unrecognized line %ld: %s\n", line_number, text); /* Show input. */
        }                                       /* End handling the current command. */
    }                                           /* End processing input lines. */
}                                               /* End reading and processing the input stream. */

/* Author: Heet Jani. Initializes resources, processes input, and cleans up. */
int main(int argc, char *argv[]) /* Start the program and coordinate all processing. */
{ /* Begin the program entry point. */
    MaxHeap heap;                              /* Stores all emails awaiting processing. */
    FILE *in = stdin;                          /* Use standard input unless a file is supplied. */

    if (argc > 2) {                            /* Begin handling too many command-line arguments. */
        fprintf(stderr, "Usage: %s [input-file]\n", argv[0]); /* Show proper program usage. */
        return 1;                              /* Stop because too many arguments were given. */
    }                                          /* End handling too many command-line arguments. */

    if (argc == 2) {                            /* Begin handling an optional input filename. */
        in = fopen(argv[1], "r");              /* Open the requested file for reading. */
        if (in == NULL) {                       /* Begin handling an input-file open failure. */
            fprintf(stderr, "Error: could not open '%s'.\n", argv[1]); /* Report the path. */
            return 1;                           /* Stop because no input can be processed. */
        }                                      /* End handling an input-file open failure. */
    }                                          /* End handling an optional input filename. */

    heap_init(&heap);                           /* Initialize the email priority queue. */
    process_stream(in, &heap);                  /* Process commands from the selected input. */
    heap_free(&heap);                           /* Release all heap memory before exiting. */

    if (in != stdin)                            /* Begin closing a file that was opened. */
        fclose(in);                             /* Close the file while preserving stdin. */

    return 0;                                   /* Report successful program completion. */
}                                               /* End the program entry point. */