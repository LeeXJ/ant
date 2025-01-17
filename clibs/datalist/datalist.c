#define LUA_LIB

#include <lua.h>
#include <lauxlib.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_DEPTH 256
#define SHORT_STRING 1024
#define CONVERTER 2
#define REF_CACHE 3
#define REF_UNSOLVED 4
#define TAB_SPACE 4

typedef uintptr_t objectid;

enum token_type {
	TOKEN_OPEN,	// 0 { [
	TOKEN_CLOSE,	// 1 } ]
	TOKEN_CONVERTER, // 2 $ ( $name something == [name, something] )
	TOKEN_MAP,	// 3 = :
	TOKEN_LIST,	// 4 ---
	TOKEN_STRING,	// 5
	TOKEN_ESCAPESTRING,	// 6
	TOKEN_ATOM,	// 7
	TOKEN_NEWLINE,	// 8 space \t
	TOKEN_TAG,	// 9	&badf00d  (64bit hex number)
	TOKEN_REF,	// 10	*badf00d
	TOKEN_EOF,	// 11 end of file
};

struct token {
	enum token_type type;
	ptrdiff_t from;
	ptrdiff_t to;
};

struct lex_state {
	const char *source;
	size_t sz;
	ptrdiff_t position;
	struct token c;
	struct token n;
	int newline;
	int aslist;
};

static inline int
inset(const char *set, char c) {
	return c != '\0' && strchr(set, c) != NULL;
}

static const char *
skip_line_comment(struct lex_state *LS) {
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	while (ptr < endptr) {
		if (*ptr == '\r' || *ptr == '\n') {
			LS->position = ptr - LS->source;
			LS->newline = 1;
			return ptr;
		}
		++ptr;
	}
	return ptr;
}

static const char *
parse_ident(struct lex_state *LS) {
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	while (ptr < endptr) {
		switch (*ptr) {
		case '\r':
		case '\n':
			LS->newline = 1;
			return ptr+1;
		case '#':
			// empty line
			return ptr;
		case ' ':
		case '\t':
			break;
		default:
			LS->n.type = TOKEN_NEWLINE;
			LS->n.from = LS->position;
			LS->n.to = ptr - LS->source;
			LS->position = LS->n.to;
			return NULL;
		}
		++ptr;
	}
	return ptr;
}

static int
is_hexnumber(struct lex_state *LS) {
	const char * ptr = LS->source + LS->n.from + 1;
	const char * endptr = LS->source + LS->n.to;
	if (ptr == endptr)
		return 0;
	do {
		char c = *ptr;
		if (!((c >= '0' && c <= '9')
			|| (c >= 'a' && c <= 'f')
			|| (c >= 'A' && c <= 'F')))
			return 0;
	} while (++ptr < endptr);
	return 1;
}

static void
parse_atom(struct lex_state *LS) {
	static const char * separator = " \t\r\n,{}[]$:\"'";
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	char head = *ptr;
	LS->n.from = LS->position;
	while (ptr < endptr) {
		if (inset(separator, *ptr)) {
			break;
		}
		++ptr;
	}
	LS->n.to = ptr - LS->source;
	LS->position = LS->n.to;
	switch (head) {
	case '&':
		if (is_hexnumber(LS)) {
			LS->n.type = TOKEN_TAG;
			return;
		}
		break;
	case '*':
		if (is_hexnumber(LS)) {
			LS->n.type = TOKEN_REF;
			return;
		}
		break;
	default:
		break;
	}
	LS->n.type = TOKEN_ATOM;
}

static int
parse_string(struct lex_state *LS) {
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	char open_string = *ptr++;
	LS->n.type = TOKEN_STRING;
	LS->n.from = LS->position + 1;
	while (ptr < endptr) {
		char c = *ptr;
		if (c == open_string) {
			LS->n.to = ptr - LS->source;
			LS->position = ptr - LS->source + 1;
			return 1;
		}
		if (c == '\r' || c == '\n') {
			return 0;
		}
		if (c == '\\') {
			LS->n.type = TOKEN_ESCAPESTRING;
			++ptr;
		}
		++ptr;
	}
	return 0;
}

// 0 : invalid source
// 1 : ok
static int
next_token(struct lex_state *LS) {
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	while (ptr < endptr) {
		LS->position = ptr - LS->source;
		if (LS->newline) {
			// line head
			LS->newline = 0;
			const char * nextptr = parse_ident(LS);
			if (nextptr == NULL)
				return 1;
			// empty line
			ptr = nextptr;
			continue;
		}

		switch (*ptr) {
		case '#':
			// comment
			ptr = skip_line_comment(LS);
			continue;
		case '\r':
		case '\n':
			LS->newline = 1;
			++ptr;
			continue;
		case ' ':
		case '\t':
		case ',':
			break;
		case '{':
		case '[':
			LS->n.type = TOKEN_OPEN;
			LS->n.from = LS->position;
			LS->n.to = ++LS->position;
			return 1;
		case '$':
			LS->n.type = TOKEN_CONVERTER;
			LS->n.from = LS->position;
			LS->n.to = ++LS->position;
			return 1;
		case '}':
		case ']':
			LS->n.type = TOKEN_CLOSE;
			LS->n.from = LS->position;
			LS->n.to = ++LS->position;
			return 1;
		case '-':
			do ++ptr; while (ptr < endptr && *ptr == '-');
			if (ptr >= endptr || inset(" \t\r\n", *ptr)) {
				LS->n.type = TOKEN_LIST;
				LS->n.from = LS->position;
				LS->n.to = ptr - LS->source;
				LS->position = LS->n.to;
			} else {
				// negative number
				parse_atom(LS);
			}
			return 1;
		case ':':
			LS->n.type = TOKEN_MAP;
			LS->n.from = LS->position;
			LS->n.to = ++LS->position;
			return 1;
		case '"':
		case '\'':
			return parse_string(LS);
		default:
			parse_atom(LS);
			return 1;
		}
		++ptr;
	}
	LS->n.type = TOKEN_EOF;
	LS->position = LS->sz;
	return 1;
}


static int
invalid(lua_State *L, struct lex_state *LS, const char * err) {
	ptrdiff_t index;
	int line = 1;
	ptrdiff_t position = LS->n.from;
	for (index = 0; index < position ; index ++) {
		if (LS->source[index] == '\n')
			++line;
	}
	return luaL_error(L, "Line %d : %s", line, err);
}

static inline int
read_token(lua_State *L, struct lex_state *LS) {
	if (LS->c.type == TOKEN_EOF) {
		invalid(L, LS, "End of data");
	}
	LS->c = LS->n;
	if (!next_token(LS))
		invalid(L, LS, "Invalid token");
//	printf("token %d %.*s\n", LS->c.type, (int)(LS->c.to-LS->c.from), LS->source + LS->c.from);
	return LS->c.type;
}

static inline int
to_hex(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int
push_token_string(lua_State *L, const char *ptr, size_t sz) {
	char tmp[SHORT_STRING];
	char *buffer = tmp;
	tmp[0] = '\0';
	assert(sz > 0);
	if (sz > SHORT_STRING) {
		buffer = lua_newuserdatauv(L, sz, 0);
	}

	size_t i, n;
	for (n=i=0;i<sz;++i,++ptr,++n) {
		if (*ptr != '\\') {
			buffer[n] = *ptr;
		} else {
			++ptr;
			++i;
			assert(i < sz);
			char c = *ptr;
			if (c == '0') {
				buffer[n] = '\0';
			} else if (c >= '1' && c <= '9') {
				// escape dec ascii
				int dec = c - '0';
				if (i+1 < sz) {
					int c2 = ptr[1];
					if (c2 >= '0' && c2 <= '9') {
						dec = dec * 10 + c2 - '0';
						++ptr;
						++i;
					}
				}
				if (i+1 < sz) {
					int c2 = ptr[1];
					if (c2 >= '0' && c2 <= '9') {
						int tmp = dec * 10 + c2 - '0';
						if (tmp <= 255) {
							dec = tmp;
							++ptr;
							++i;
						}
					}
				}
				buffer[n] = dec;
			} else {
				switch(*ptr) {
				case 'x':
				case 'X': {
					// escape hex ascii
					if (i+2 >= sz) {
						return 1;
					}
					++ptr;
					++i;
					int hex = to_hex(*ptr);
					if (hex < 0) {
						return 1;
					}
					++ptr;
					++i;
					int hex2 = to_hex(*ptr);
					if (hex2 >= 0) {
						hex = hex * 16 + hex2;
					}
					buffer[n] = hex;
					break;
				}
				case 'n':
					buffer[n] = '\n';
					break;
				case 'r':
					buffer[n] = '\r';
					break;
				case 't':
					buffer[n] = '\t';
					break;
				case 'a':
					buffer[n] = '\a';
					break;
				case 'b':
					buffer[n] = '\b';
					break;
				case 'v':
					buffer[n] = '\v';
					break;
				case '\'':
				case '"':
				case '\n':
				case '\r':
					buffer[n] = *ptr;
					break;
				default:
					return 1;
				}
			}
		}
	}
	lua_pushlstring(L, buffer, n);
	if (sz > SHORT_STRING) {
		lua_replace(L, -2);
	}
	return 0;
}

#define IS_KEYWORD(ptr, sz, str) (sizeof(str "") == sz+1 && (memcmp(ptr, str, sz) == 0))

static void
push_token(lua_State *L, struct lex_state *LS, struct token *t) {
	const char * ptr = LS->source + t->from;
	size_t sz = t->to - t->from;

	switch(t->type) {
	case TOKEN_STRING:
		lua_pushlstring(L, ptr, sz);
		return;
	case TOKEN_ESCAPESTRING:
		if (push_token_string(L, ptr, sz)) {
			invalid(L, LS, "Invalid quote string");
		}
		return;
	case TOKEN_ATOM:
		break;
	default:
		invalid(L, LS, "Invalid atom");
		return;
	}

	if (inset("0123456789+-.", ptr[0])) {
		if (sz == 1) {
			char c = *ptr;
			if (c >= '0' && c <='9') {
				lua_pushinteger(L, c - '0');
			} else {
				lua_pushlstring(L, ptr, 1);
			}
			return;
		}

		if (sz >=3 && ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
			// may be a hex integer
			lua_Integer v = 0;
			int hex = 1;
			size_t i;
			for (i=2;i<sz;i++) {
				char c = ptr[i];
				v = v * 16;
				if (c >= '0' && c <='9') {
					v += c - '0';
				} else if (c >= 'a' && c <= 'f') {
					v += c - 'a' + 10;
				} else if (c >= 'A' && c <= 'F') {
					v += c - 'A' + 10;
				} else {
					hex = 0;
					break;
				}
			}
			if (hex) {
				lua_pushinteger(L, v);
				return;
			}
		}

		// may be a number
		// lua string always has \0 at the end, so strto* is safe
		char *endptr = NULL;
		lua_Integer v = strtoull(ptr, &endptr, 10);
		if (endptr - ptr == sz) {
			lua_pushinteger(L, v);
			return;
		}

		endptr = NULL;
		lua_Number f = strtod(ptr, &endptr);
		if (endptr - ptr == sz) {
			lua_pushnumber(L, f);
			return;
		}
	}

	if (t->type == TOKEN_ATOM) {
		if (IS_KEYWORD(ptr, sz, "true")) {
			lua_pushboolean(L, 1);
			return;
		} else if (IS_KEYWORD(ptr, sz, "false")) {
			lua_pushboolean(L, 0);
			return;
		} else if (IS_KEYWORD(ptr, sz, "nil")) {
			lua_pushnil(L);
			return;
		}
	}

	lua_pushlstring(L, ptr, sz);
}

static inline int
token_length(struct token *t) {
	return (int)(t->to - t->from);
}

static inline int
token_ident(struct lex_state *LS) {
	struct token *t = &LS->c;
	const char * ptr = LS->source + t->from;
	const char * endptr = LS->source + t->to;
	int ident = token_length(t);
	while (endptr > ptr) {
		if (*ptr == '\t')
			ident += TAB_SPACE - 1;
		++ptr;
	}
	return ident;
}

static inline int
token_symbol(struct lex_state *LS) {
	return LS->source[LS->c.from];
}

static inline void
push_key(lua_State *L, struct lex_state *LS) {
	lua_pushlstring(L, LS->source + LS->c.from, LS->c.to - LS->c.from);
}

static inline void
new_table_0(lua_State *L) {
	lua_newtable(L);
	// index 0 refer self
	// lua_pushvalue(L, -1);
	// lua_rawseti(L, -2, 0);
}

static void
new_table(lua_State *L, int layer, objectid ref) {
	if (layer >= MAX_DEPTH)
		luaL_error(L, "too many layers");
	luaL_checkstack(L, 8, NULL);
	if (ref == 0) {
		new_table_0(L);
	} else {
		lua_rawgeti(L, REF_CACHE, ref);
	}
}

static objectid
read_tag(struct lex_state *LS) {
	const char * ptr = LS->source + LS->c.from + 1;
	const char * endptr = LS->source + LS->c.to;
	objectid x = 0;
	while (ptr < endptr) {
		char c = *ptr;
		int n;
		if (c >= '0' && c <= '9')
			n = c - '0';
		else if (c >= 'a' && c <= 'f')
			n = c - 'a' + 10;
		else
			n = c - 'A' + 10;
		x = x * 16 + n;
		++ptr;
	}
	if (x == 0) {
		x = ~(objectid)0;
	}
	return x;
}

static objectid
parse_tag(lua_State *L, struct lex_state *LS) {
	objectid tag = read_tag(LS);
	if (lua_rawgeti(L, REF_CACHE, tag) != LUA_TNIL) {
		lua_pop(L, 1);
		if (lua_rawgeti(L, REF_UNSOLVED, tag) == LUA_TNIL) {
			invalid(L, LS, "Duplicate tag");
		}
		lua_pop(L, 1);
		// clear unsolved
		lua_pushnil(L);
		lua_rawseti(L, REF_UNSOLVED, tag);
	} else {
		lua_pop(L, 1);
		new_table_0(L);
		lua_rawseti(L, REF_CACHE, tag);
	}
	return tag;
}

static void
parse_ref(lua_State *L, struct lex_state *LS) {
	objectid tag = read_tag(LS);
	read_token(L, LS);	//	consume ref tag
	if (lua_rawgeti(L, REF_CACHE, tag) != LUA_TNIL) {
		return;
	}
	lua_pop(L, 1);
	new_table_0(L);		// Create a table for future
	lua_pushvalue(L, -1);
	lua_rawseti(L, REF_CACHE, tag);
	// set unsolved flag
	lua_pushboolean(L, 1);
	lua_rawseti(L, REF_UNSOLVED, tag);
}

static void parse_bracket(lua_State *L, struct lex_state *LS, int layer, objectid tag);
static void parse_converter(lua_State *L, struct lex_state *LS, int layer, int ident);

static int
closed_bracket(lua_State *L, struct lex_state *LS, int bracket) {
	for (;;) {
		switch (LS->c.type) {
		case TOKEN_CLOSE:
			if (token_symbol(LS) != bracket) {
				invalid(L, LS, "Invalid close bracket");
			}
			read_token(L, LS);
			return 1;
		case TOKEN_NEWLINE:
			read_token(L, LS);
			break;
		default:
			return 0;
		}
	}
}

// table key value
static void
set_keyvalue(lua_State *L) {
	lua_pushvalue(L, -2);
	// table key value key
	int oldv = lua_gettable(L, -4);
	// table key value oldv
	if (oldv == LUA_TNIL) {
		lua_pop(L, 1);
		lua_settable(L, -3);
	} else if (oldv == LUA_TTABLE) {
		lua_len(L, -1);
		int n = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);
		lua_replace(L, -3);
		// table oldv value
		lua_seti(L, -2, n+1);
		lua_pop(L, 1);
	} else {
		luaL_error(L, "Multi-key (%s) should be a table", lua_tostring(L, -3));
	}
}

static void
parse_bracket_map(lua_State *L, struct lex_state *LS, int layer, int bracket) {
	int i = 1;
	int aslist = LS->aslist;
	do {
		if (LS->c.type != TOKEN_ATOM) {
			invalid(L, LS, "Invalid key");
		}
		push_key(L, LS);
		if (read_token(L, LS) != TOKEN_MAP) {
			invalid(L, LS, "Need a : or =");
		}
		objectid tag = 0;
		int t = read_token(L, LS);
		if (t == TOKEN_TAG) {
			tag = parse_tag(L, LS);
			t = read_token(L, LS);
		}
		while (t == TOKEN_NEWLINE) {
			t = read_token(L, LS);
		}
		switch (LS->c.type) {
		case TOKEN_REF:
			if (tag != 0) {
				invalid(L, LS, "Invalid ref in bracket map");
			}
			parse_ref(L, LS);
			break;
		case TOKEN_OPEN:
			parse_bracket(L, LS, layer+1, tag);
			break;
		case TOKEN_CONVERTER:
			parse_converter(L, LS, layer+1, -1);
			break;
		default:
			push_token(L, LS, &LS->c);
			read_token(L, LS);
			break;
		}
		if (aslist) {
			lua_insert(L, -2);
			lua_seti(L, -3, i++);
			lua_seti(L, -2, i++);
		} else {
			set_keyvalue(L);
		}
	} while (!closed_bracket(L, LS, bracket));
}

static void
parse_bracket_sequence(lua_State *L, struct lex_state *LS, int layer, int bracket) {
	int n = 1;
	for (;;) {
		switch (LS->c.type) {
		case TOKEN_NEWLINE:
			read_token(L, LS);
			continue;	// skip ident
		case TOKEN_CLOSE:
			if (token_symbol(LS) != bracket) {
				invalid(L, LS, "Invalid close bracket");
			}
			read_token(L, LS);	// consume }
			return;
		case TOKEN_REF:
			parse_ref(L, LS);
			return;
		case TOKEN_OPEN:
			// No tag in sequence
			parse_bracket(L, LS, layer, 0);
			break;
		case TOKEN_CONVERTER:
			parse_converter(L, LS, layer, -1);
			break;
		default:
			push_token(L, LS, &LS->c);
			read_token(L, LS);
			break;
		}
		lua_seti(L, -2, n++);
	}
}

static inline void
parse_bracket_(lua_State *L, struct lex_state *LS, int layer, objectid ref, int bracket) {
	new_table(L, layer, ref);
again:
	switch (read_token(L, LS)) {
	case TOKEN_NEWLINE:
		goto again;
	case TOKEN_CLOSE:
		if (token_symbol(LS) != bracket) {
			invalid(L, LS, "Invalid close bracket");
		}
		read_token(L, LS);	// consume }
		return;
	case TOKEN_ATOM:
		if (LS->n.type == TOKEN_MAP) {
			parse_bracket_map(L, LS, layer, bracket);
			return;
		}
		break;
	default:
		break;
	}
	parse_bracket_sequence(L, LS, layer, bracket);
}

static void 
parse_bracket(lua_State *L, struct lex_state *LS, int layer, objectid ref) {
	char bracket = token_symbol(LS);
	if (bracket == '[') {
		if (ref) {
			invalid(L, LS, "[] can't has a tag");
		}
		parse_bracket_(L, LS, layer, ref, ']');
		lua_pushvalue(L, CONVERTER);
		lua_insert(L, -2);
		lua_call(L, 1, 1);
	} else {
		parse_bracket_(L, LS, layer, ref, '}');
	}
}

static void parse_section(lua_State *L, struct lex_state *LS, int layer);

static void
parse_converter(lua_State *L, struct lex_state *LS, int layer, int ident) {
	new_table(L, layer, 0);
	if (read_token(L, LS) != TOKEN_ATOM) {
		invalid(L, LS, "$ need an atom");
	}
	push_key(L, LS);
	lua_rawseti(L, -2, 1);	// $atom xxx === [ "atom",  xxx ]

	read_token(L, LS);

	switch (LS->c.type) {
	case TOKEN_NEWLINE:
		if (ident < 0)
			invalid(L, LS, "Invalid newline , Use { } for a struct instead");
		int next_ident = token_ident(LS);
		if (next_ident < ident) {
			invalid(L, LS, "Invalid new section ident");
		}
		new_table(L, layer+1, 0);
		parse_section(L, LS, layer+1);
		break;
	case TOKEN_CLOSE:
		invalid(L, LS, "Invalid close bracket");
		break;
	case TOKEN_REF:
		parse_ref(L, LS);
		break;
	case TOKEN_OPEN:
		parse_bracket(L, LS, layer, 0);
		break;
	case TOKEN_CONVERTER:
		parse_converter(L, LS, layer, ident+1);
		break;
	default:
		push_token(L, LS, &LS->c);
		read_token(L, LS);
		break;
	}

	lua_seti(L, -2, 2);
	lua_pushvalue(L, CONVERTER);
	lua_insert(L, -2);
	lua_call(L, 1, 1);
}

static int
next_item(lua_State *L, struct lex_state *LS, int ident) {
	int t = LS->c.type;
	if (t == TOKEN_NEWLINE) {
		int next_ident = token_ident(LS);
		if (next_ident == ident) {
			if (LS->n.type == TOKEN_LIST)
				return 0;
			read_token(L, LS);
			return 1;
		} else if (next_ident > ident) {
			invalid(L, LS, "Invalid ident");
		} else {
			// end of sequence
			return 0;
		}
	} else if (t == TOKEN_EOF) {
		return 0;
	}
	return 1;
}

static void
parse_section_map(lua_State *L, struct lex_state *LS, int ident, int layer) {
	int i = 1;
	int aslist = LS->aslist;
	do {
		if (LS->c.type != TOKEN_ATOM)
			invalid(L, LS, "Invalid key");
		push_key(L, LS);
		if (read_token(L, LS) != TOKEN_MAP) {
			invalid(L, LS, "Need a : or =");
		}
		objectid tag = 0;
		int t = read_token(L, LS);
		if (t == TOKEN_TAG) {
			tag = parse_tag(L, LS);
			t = read_token(L, LS);
		}

		switch (t) {
		case TOKEN_REF:
			if (tag != 0) {
				invalid(L, LS, "Invalid ref after tag");
			}
			parse_ref(L, LS);
			break;
		case TOKEN_OPEN:
			parse_bracket(L, LS, layer+1, tag);
			break;
		case TOKEN_CONVERTER:
			parse_converter(L, LS, layer+1, ident+1);
			break;
		case TOKEN_NEWLINE: {
			int next_ident = token_ident(LS);
			if (next_ident <= ident) {
				invalid(L, LS, "Invalid new section ident");
			}
			new_table(L, layer+1, tag);
			parse_section(L, LS, layer+1);
			break;
		}
		default:
			push_token(L, LS, &LS->c);
			read_token(L, LS);
			break;
		}
		if (aslist) {
			lua_insert(L, -2);
			lua_seti(L, -3, i++);
			lua_seti(L, -2, i++);
		} else {
			set_keyvalue(L);
		}
	} while (next_item(L, LS, ident));
}

static void
parse_section_sequence(lua_State *L, struct lex_state *LS, int ident, int layer) {
	int n = 1;
	do {
		switch (LS->c.type) {
		case TOKEN_REF:
			parse_ref(L, LS);
			break;
		case TOKEN_OPEN:
			parse_bracket(L, LS, layer+1, 0);
			break;
		case TOKEN_CONVERTER:
			parse_converter(L, LS, layer+1, ident+1);
			break;
		case TOKEN_LIST:
			// end of this section
			return;
		default:
			push_token(L, LS, &LS->c);
			read_token(L, LS);
			break;
		}
		lua_seti(L, -2, n++);
	} while(next_item(L, LS, ident));
}

static int
next_list(lua_State *L, struct lex_state *LS, int ident) {
	int t = LS->c.type;
	if (t == TOKEN_NEWLINE) {
		int next_ident = token_ident(LS);
		if (next_ident == ident) {
			switch (read_token(L, LS)) {
			case TOKEN_EOF:
				return 0;
			case TOKEN_LIST:
				// next list
				return 1;
			default:
				invalid(L, LS, "Invalid list");
				break;
			}
		} else if (next_ident < ident) {
			// end of sequence
			return 0;
		}
	} else if (t == TOKEN_EOF)
		return 0;
	invalid(L, LS, "Invalid list");
	return 0;
}

static void
empty_list(lua_State *L, int n, objectid tag) {
	new_table(L, 0, 0);
	lua_pushvalue(L, -1);
	lua_rawseti(L, REF_CACHE, tag);
	lua_seti(L, -2, n);
}

static void
parse_section_list(lua_State *L, struct lex_state *LS, int ident, int layer) {
	int n = 1;
	do {
		int t = read_token(L, LS);
		objectid tag = 0;
		if (t == TOKEN_TAG) {
			tag = parse_tag(L, LS);
			t = read_token(L, LS);
		}
		switch (t) {
		case TOKEN_REF:
			if (tag != 0) {
				invalid(L, LS, "Invalid ref after tag");
			}
			parse_ref(L, LS);
			break;
		case TOKEN_OPEN:
			parse_bracket(L, LS, layer+1, tag);
			break;
		case TOKEN_CONVERTER:
			parse_converter(L, LS, layer+1, ident);
			break;
		case TOKEN_NEWLINE: {
			int next_ident = token_ident(LS);
			if (next_ident >= ident) {
				new_table(L, layer + 1, tag);
				if (LS->n.type != TOKEN_LIST || next_ident > ident) {
					// not an empty list
					parse_section(L, LS, layer + 1);
				}
			} else {
				// end of list
				empty_list(L, n, tag);
				return;
			}
			break;
		}
		case TOKEN_EOF:
			// empty list
			empty_list(L, n, tag);
			return;
		default:
			push_token(L, LS, &LS->c);
			read_token(L, LS);
			break;
		}
		lua_seti(L, -2, n++);
	} while(next_list(L, LS, ident));
}

static void
parse_section(lua_State *L, struct lex_state *LS, int layer) {
	int ident = token_ident(LS);
	switch (read_token(L, LS)) {
	case TOKEN_ATOM:
		if (LS->n.type == TOKEN_MAP) {
			parse_section_map(L, LS, ident, layer);
			return;
		}
		break;
	case TOKEN_EOF:
		return;
	case TOKEN_STRING:
	case TOKEN_ESCAPESTRING:
	case TOKEN_OPEN:
	case TOKEN_REF:
	case TOKEN_CONVERTER:
		break;
	case TOKEN_LIST:
		parse_section_list(L, LS, ident, layer);
		return;
	default:
		invalid(L, LS, "Invalid section");
	}
	// a sequence
	parse_section_sequence(L, LS, ident, layer);
}

static void
init_lex(lua_State *L, int index, struct lex_state *LS) {
	switch (lua_type(L, 1)) {
	case LUA_TUSERDATA:
		LS->source = (const char*)lua_touserdata(L, 1);
		LS->sz = lua_rawlen(L, 1);
		break;
	case LUA_TFUNCTION: {
		lua_pushvalue(L, 1);
		lua_call(L, 0, 3);
		LS->source = (const char*)lua_touserdata(L, -3);
		LS->sz = (size_t)luaL_checkinteger(L, -2);
		lua_copy(L, -1, 1);
		int t = lua_type(L, 1);
		if (t == LUA_TUSERDATA || t == LUA_TTABLE)
			lua_toclose(L, 1);
		lua_pop(L, 3);
		break;
	}
	default:
	case LUA_TSTRING:
		LS->source = luaL_checklstring(L, 1, &LS->sz);
		break;
	}
	LS->position = 0;
	LS->newline = 1;
	LS->aslist = 0;
	if (!next_token(LS))
		invalid(L, LS, "Invalid token");
}

static int
dummy_converter(lua_State *L) {
	return 1;
}

static void
parse_all(lua_State *L, struct lex_state *LS) {
	int t = lua_type(L, 2);
	if (t != LUA_TFUNCTION) {
		lua_pushcfunction(L, dummy_converter);
		lua_insert(L, 2);
	} else {
		t = lua_type(L, 3);
	}
	if (t == LUA_TTABLE || t == LUA_TUSERDATA) {
		lua_settop(L, 3);
	} else {
		new_table(L, 0, 0);
	}
	lua_newtable(L);	// ref cache (index 3/REF_CACHE)
	lua_newtable(L);	// unsolved ref (index 4/REF_UNSOLVED)
	lua_rotate(L, -3, 2);
	int tt = read_token(L, LS);
	if (tt == TOKEN_EOF)
		return;
	assert(tt == TOKEN_NEWLINE);
	parse_section(L, LS, 0);
	if (LS->c.type != TOKEN_EOF) {
		invalid(L, LS, "not end");
	}
	// check unsolved
	lua_pushnil(L);
	if (lua_next(L, REF_UNSOLVED) != 0) {
		objectid tag = lua_tointeger(L, -2);
		luaL_error(L, "Unsolved tag %p", (void *)tag);
	}
}

static int
lparse(lua_State *L) {
	struct lex_state LS;
	init_lex(L, 1, &LS);
	parse_all(L, &LS);
	lua_pushvalue(L, REF_CACHE);
	return 2;
}

static int
lparse_list(lua_State *L) {
	struct lex_state LS;
	init_lex(L, 1, &LS);
	LS.aslist = 1;
	parse_all(L, &LS);
	lua_pushvalue(L, REF_CACHE);
	return 2;
}

static int
ltoken(lua_State *L) {
	struct lex_state LS;
	init_lex(L, 1, &LS);

	lua_newtable(L);

	int n = 1;
	while(read_token(L, &LS) != TOKEN_EOF) {
		lua_pushlstring(L, LS.source + LS.c.from, LS.c.to - LS.c.from);
		lua_seti(L, -2, n++);
	}
	return 1;
}

static inline int
utf8_trail(unsigned char c) {
	return c >= 0x80 && c <= 0xbf;
}

static size_t
valid_utf8(const char *str, size_t sz) {
	unsigned char c = *str;
	if (c >= 0xc0 && c <= 0xdf) {
		// 110xxxxx  10zzzzzz (0x80 - 0xbf)
		if (sz < 2)
			return 0;
		if (!utf8_trail(str[1]))
			return 0;
		return 2;
	} else if (c >= 0xe0 && c <= 0xef) {
		// 1110xxxx  10zzzzzz 10zzzzzz
		if (sz < 3)
			return 0;
		if (!utf8_trail(str[1]) || !utf8_trail(str[2]))
			return 0;
		return 3;
	} else if (c >= 0xf0 && c <= 0xf7) {
		// 11110xxx  10zzzzzz 10zzzzzz 10zzzzzz
		if (sz < 4)
			return 0;
		if (!utf8_trail(str[1]) || !utf8_trail(str[2]) || !utf8_trail(str[3]))
			return 0;
		return 4;
	} else if (c < 128)
		return 1;
	return 0;
}

static void
add_hex(luaL_Buffer *b, unsigned char c) {
	char tmp[5];
	snprintf(tmp, sizeof(tmp), "\\x%02X", c);
	luaL_addstring(b, tmp);
}

static int
lquote(lua_State *L) {
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	luaL_addchar(&b, '"');
	size_t sz,i;
	const char * str = luaL_checklstring(L, 1, &sz);
	for (i=0;i<sz;i++) {
		unsigned char c = (unsigned char)str[i];
		if (c < 32) {
			switch (c) {
			case 0:
				luaL_addchar(&b, '\\');
				luaL_addchar(&b, '0');
				break;
			case '\t':
				luaL_addchar(&b, '\\');
				luaL_addchar(&b, 't');
				break;
			case '\n':
				luaL_addchar(&b, '\\');
				luaL_addchar(&b, 'n');
				break;
			case '\r':
				luaL_addchar(&b, '\\');
				luaL_addchar(&b, 'r');
				break;
			default:
				add_hex(&b, c);
				break;
			}
		} else if (c == '"') {
			luaL_addchar(&b, '\\');
			luaL_addchar(&b, '"');
		} else if (c == '\\') {
			luaL_addchar(&b, '\\');
			luaL_addchar(&b, '\\');
		} else if (c >= 128) {
			// check utf-8
			int n = valid_utf8(str+i, sz-i);
			if (n == 0) {
				add_hex(&b, c);
			} else {
				luaL_addlstring(&b,str+i,n);
				i += n-1;
			}
		} else {
			luaL_addchar(&b, c);
		}
	}
	luaL_addchar(&b, '"');
	luaL_pushresult(&b);
	return 1;
}

LUAMOD_API int
luaopen_datalist(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "parse", lparse },
		{ "parse_list", lparse_list },
		{ "token", ltoken },
		{ "quote", lquote },
		{ NULL, NULL },
	};

	luaL_newlib(L, l);

	return 1;
}

// 该代码是一个用C语言编写的解析器，用于解析一种类似数据交换格式的文本输入。下面是代码中的主要功能：

// 1. 定义了一些常量和枚举，如TOKEN_OPEN、TOKEN_CLOSE等，用于表示不同的标记类型。
// 2. 实现了一系列辅助函数，如inset、skip_line_comment等，用于处理输入文本。
// 3. 实现了一个词法分析器，将输入文本分解为标记序列。
// 4. 实现了一个语法解析器，根据标记序列构建相应的数据结构。
// 5. 支持处理不同类型的标记，如字符串、转义字符串、原子值、换行符、标签、引用等。
// 6. 能够识别特定语法结构，如列表、映射等，并构建对应的数据结构。
// 7. 支持标签和引用，可以在不同位置引用相同的数据。
// 8. 支持嵌套结构和层级标识。

// 这个解析器的主要目的是将文本数据转换为内部的数据结构，以供程序进一步处理和操作。