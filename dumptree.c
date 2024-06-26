/* markdown: a C implementation of John Gruber's Markdown markup language.
 *
 * Copyright (C) 2007 Jessica L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <stdio.h>
#include "markdown.h"
#include "cstring.h"
#include "amalloc.h"

struct frame {
    int indent;
    char c;
};

typedef STRING(struct frame) Stack;

static char *
Pptype(int typ)
{
    switch (typ) {
    case WHITESPACE: return "whitespace";
    case CODE      : return "code";
    case QUOTE     : return "quote";
    case MARKUP    : return "markup";
    case HTML      : return "html";
    case DL        : return "dl";
    case UL        : return "ul";
    case OL        : return "ol";
    case LISTITEM  : return "item";
    case HDR       : return "header";
    case HR        : return "hr";
    case TABLE     : return "table";
    case SOURCE    : return "source";
    case STYLE     : return "style";
    default        : return "mystery node!";
    }
}

static void
pushpfx(int indent, char c, Stack *sp)
{
    struct frame *q = &EXPAND(*sp);

    q->indent = indent;
    q->c = c;
}


static void
poppfx(Stack *sp)
{
    S(*sp)--;
}


static void
changepfx(Stack *sp, char c)
{
    char ch;

    if ( S(*sp) > 0 ) {
	ch = T(*sp)[S(*sp)-1].c;

	if ( ch == '+' || ch == '|' )
	    T(*sp)[S(*sp)-1].c = c;
    }
}


static void
printpfx(Stack *sp, FILE *f)
{
    int i;
    char c;

    if ( S(*sp) > 0 ) {
	c = T(*sp)[S(*sp)-1].c;

	if ( c == '+' || c == '-' ) {
	    fprintf(f, "--%c", c);
	    T(*sp)[S(*sp)-1].c = (c == '-') ? ' ' : '|';
	}
	else
	    for ( i=0; i < S(*sp); i++ ) {
		if ( i )
		    fprintf(f, "  ");
		fprintf(f, "%*s%c", T(*sp)[i].indent + 2, " ", T(*sp)[i].c);
		if ( T(*sp)[i].c == '`' )
		    T(*sp)[i].c = ' ';
	    }
	fprintf(f, "--");
    }
}


static void
dumptree(Paragraph *pp, Stack *sp, FILE *f)
{
    int count;
    Line *p;
    int d;
    static char *Begin[] = { 0, "P", "center" };

    while ( pp ) {
	if ( !pp->next )
	    changepfx(sp, '`');
	printpfx(sp, f);

	if ( pp->typ == HDR )
	    d += fprintf(f, "[h%d", pp->hnumber);
	else
	    d = fprintf(f, "[%s", Pptype(pp->typ));
	if ( pp->ident )
	    d += fprintf(f, " %s", pp->ident);

	if ( pp->para_flags )
	    d += fprintf(f, " %x", pp->para_flags);

	if ( pp->align > 1 )
	    d += fprintf(f, ", <%s>", Begin[pp->align]);

	for (count=0, p=pp->text; p; ++count, (p = p->next) )
	    ;

	if ( count )
	    d += fprintf(f, ", %d line%s", count, (count==1)?"":"s");

#if EXTENDED_DEBUG
	if ( pp->text && T(pp->text->text) )
	    d += fprintf(f, " <%.*s>", S(pp->text->text), T(pp->text->text));
#endif

	d += fprintf(f, "]");

	if ( pp->down ) {
	    pushpfx(d, pp->down->next ? '+' : '-', sp);
	    dumptree(pp->down, sp, f);
	    poppfx(sp);
	}
	else fputc('\n', f);
	pp = pp->next;
    }
}


int
mkd_dump(Document *doc, FILE *out, mkd_flag_t *flags, char *title)
{
    Stack stack;

    if ( mkd_compile(doc, flags) && doc->code ) {

	CREATE(stack);
	pushpfx(fprintf(out, "%s", title), doc->code->next ? '+' : '-', &stack);
	dumptree(doc->code, &stack, out);
	DELETE(stack);

	return 0;
    }
    return -1;
}
