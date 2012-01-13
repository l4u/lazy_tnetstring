#ifndef TERM_H
#define TERM_H

typedef char Term;

typedef enum
{
	TYPE_STRING = ',',
	TYPE_INTEGER = '#',
	TYPE_BOOLEAN = '!',
	TYPE_LIST = ']',
	TYPE_DICTIONARY = '}'
} TermType;


TermType term_type(Term* term);
size_t term_length(Term* term);
const char* term_payload(Term* term);

#endif
