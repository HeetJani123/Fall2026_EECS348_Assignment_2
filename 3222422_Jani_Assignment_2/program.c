#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE    1024
#define MAX_SUBJECT 512
#define MAX_SENDER  64
#define MAX_DATE    16
#define NUM_CATEGORIES 5

static const char *CATEGORY_NAMES[NUM_CATEGORIES] = {
    "Boss",
    "Subordinate",
    "Peer",
    "ImportantPerson",
    "OtherPerson"
};

typedef struct {
    int month;
    int day;
    int year;
} Date;

typedef struct {
    char sender[MAX_SENDER];
    char subject[MAX_SUBJECT];
    char date_text[MAX_DATE];
    Date date;
    int  rank;
    long sequence;
} Email;

typedef struct {
    Email *items;
    int    size;
    int    capacity;
} MaxHeap;

static int email_compare(const Email *a, const Email *b)
{
    if (a->rank != b->rank)
        return (a->rank < b->rank) ? 1 : -1;

    if (a->date.year != b->date.year)
        return (a->date.year > b->date.year) ? 1 : -1;
    if (a->date.month != b->date.month)
        return (a->date.month > b->date.month) ? 1 : -1;
    if (a->date.day != b->date.day)
        return (a->date.day > b->date.day) ? 1 : -1;

    if (a->sequence != b->sequence)
        return (a->sequence > b->sequence) ? 1 : -1;

    return 0;
}

static void heap_init(MaxHeap *h)
{
    h->items    = NULL;
    h->size     = 0;
    h->capacity = 0;
}

static void heap_free(MaxHeap *h)
{
    free(h->items);
    h->items    = NULL;
    h->size     = 0;
    h->capacity = 0;
}

static int heap_is_empty(const MaxHeap *h)
{
    return h->size == 0;
}

static int heap_count(const MaxHeap *h)
{
    return h->size;
}

static int heap_reserve(MaxHeap *h, int needed)
{
    int new_capacity;
    Email *grown;

    if (needed <= h->capacity)
        return 1;

    new_capacity = (h->capacity == 0) ? 8 : h->capacity * 2;
    while (new_capacity < needed)
        new_capacity *= 2;

    grown = (Email *)realloc(h->items, (size_t)new_capacity * sizeof(Email));
    if (grown == NULL)
        return 0;

    h->items    = grown;
    h->capacity = new_capacity;
    return 1;
}

static void heap_swap(MaxHeap *h, int i, int j)
{
    Email temp = h->items[i];
    h->items[i] = h->items[j];
    h->items[j] = temp;
}

static void sift_up(MaxHeap *h, int index)
{
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (email_compare(&h->items[index], &h->items[parent]) > 0) {
            heap_swap(h, index, parent);
            index = parent;
        } else {
            break;
        }
    }
}

static void sift_down(MaxHeap *h, int index)
{
    for (;;) {
        int left    = 2 * index + 1;
        int right   = 2 * index + 2;
        int largest = index;

        if (left < h->size &&
            email_compare(&h->items[left], &h->items[largest]) > 0)
            largest = left;

        if (right < h->size &&
            email_compare(&h->items[right], &h->items[largest]) > 0)
            largest = right;

        if (largest == index)
            break;

        heap_swap(h, index, largest);
        index = largest;
    }
}

static int heap_insert(MaxHeap *h, const Email *e)
{
    if (!heap_reserve(h, h->size + 1))
        return 0;

    h->items[h->size] = *e;
    h->size++;
    sift_up(h, h->size - 1);
    return 1;
}

static int heap_peek(const MaxHeap *h, Email *out)
{
    if (heap_is_empty(h))
        return 0;

    *out = h->items[0];
    return 1;
}

static int heap_remove_max(MaxHeap *h, Email *out)
{
    if (heap_is_empty(h))
        return 0;

    if (out != NULL)
        *out = h->items[0];

    h->items[0] = h->items[h->size - 1];
    h->size--;

    if (h->size > 0)
        sift_down(h, 0);

    return 1;
}

static char *trim(char *s)
{
    char *end;

    while (*s != '\0' && isspace((unsigned char)*s))
        s++;

    if (*s == '\0')
        return s;

    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';

    return s;
}

static void copy_bounded(char *dest, size_t dest_size, const char *src)
{
    size_t i;
    for (i = 0; i + 1 < dest_size && src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[i] = '\0';
}

static int category_rank(const char *name)
{
    int i;
    for (i = 0; i < NUM_CATEGORIES; i++) {
        if (strcmp(name, CATEGORY_NAMES[i]) == 0)
            return i;
    }
    return -1;
}

static int parse_date(const char *text, Date *out)
{
    int month = 0, day = 0, year = 0;
    char extra;

    if (sscanf(text, "%d-%d-%d%c", &month, &day, &year, &extra) != 3)
        return 0;

    if (month < 1 || month > 12 || day < 1 || day > 31 || year < 0)
        return 0;

    out->month = month;
    out->day   = day;
    out->year  = year;
    return 1;
}

static int parse_email(char *body, long sequence, Email *out)
{
    char *first_comma, *second_comma;
    char *sender, *subject, *date_text;
    int rank;

    first_comma = strchr(body, ',');
    if (first_comma == NULL)
        return 0;
    *first_comma = '\0';

    second_comma = strchr(first_comma + 1, ',');
    if (second_comma == NULL)
        return 0;
    *second_comma = '\0';

    sender    = trim(body);
    subject   = trim(first_comma + 1);
    date_text = trim(second_comma + 1);

    rank = category_rank(sender);
    if (rank < 0)
        return 0;

    if (!parse_date(date_text, &out->date))
        return 0;

    copy_bounded(out->sender,    sizeof(out->sender),    sender);
    copy_bounded(out->subject,   sizeof(out->subject),   subject);
    copy_bounded(out->date_text, sizeof(out->date_text), date_text);
    out->rank     = rank;
    out->sequence = sequence;

    return 1;
}

static void do_next(const MaxHeap *h)
{
    Email top;

    if (!heap_peek(h, &top)) {
        printf("No emails to read.\n");
        return;
    }

    printf("Next email:\n");
    printf("Sender: %s\n", top.sender);
    printf("Subject: %s\n", top.subject);
    printf("Date: %s\n", top.date_text);
}

static void do_read(MaxHeap *h)
{
    heap_remove_max(h, NULL);
}

static void do_count(const MaxHeap *h)
{
    printf("There are %d emails to read.\n", heap_count(h));
}

static void process_stream(FILE *in, MaxHeap *heap)
{
    char line[MAX_LINE];
    long line_number = 0;
    long arrival     = 0;

    while (fgets(line, sizeof(line), in) != NULL) {
        char *text;
        line_number++;

        text = trim(line);
        if (*text == '\0')
            continue;

        if (strncmp(text, "EMAIL", 5) == 0 &&
            (text[5] == '\0' || isspace((unsigned char)text[5]))) {
            Email e;
            char *body = trim(text + 5);

            if (parse_email(body, arrival, &e)) {
                arrival++;
                if (!heap_insert(heap, &e)) {
                    fprintf(stderr,
                            "Error: out of memory while storing email on line %ld.\n",
                            line_number);
                    return;
                }
            } else {
                fprintf(stderr,
                        "Warning: skipping malformed email on line %ld.\n",
                        line_number);
            }
        } else if (strcmp(text, "NEXT") == 0) {
            do_next(heap);
        } else if (strcmp(text, "READ") == 0) {
            do_read(heap);
        } else if (strcmp(text, "COUNT") == 0) {
            do_count(heap);
        } else {
            fprintf(stderr,
                    "Warning: unrecognized line %ld: %s\n", line_number, text);
        }
    }
}

int main(int argc, char *argv[])
{
    MaxHeap heap;
    FILE *in = stdin;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [input-file]\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        in = fopen(argv[1], "r");
        if (in == NULL) {
            fprintf(stderr, "Error: could not open '%s'.\n", argv[1]);
            return 1;
        }
    }

    heap_init(&heap);
    process_stream(in, &heap);
    heap_free(&heap);

    if (in != stdin)
        fclose(in);

    return 0;
}   