#include "scarbjson.h"
#include <assert.h>		// assert()
#include <stdlib.h>		// NULL

// c != ch, report error
#define EXPECT(c, ch)		do{ assert(*c->json == (ch)); c->json++; } while(0)

/* 为了减少解析函数之间传递多个参数,把这些数据都放进一个结构体 */
typedef struct
{
	const char * json;
}scarb_context;

/*
	解析空格，
*/
static void scarb_parse_whitespace(scarb_context* c)
{
	const char *p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

static int scarb_parse_true(scarb_context* c, scarb_value* v)
{
	EXPECT(c, 't');
	if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
		return SCARB_PARSE_INVALID_VALUE;
	c->json += 3;		// move through 'rue'
	v->type = SCARB_TRUE;
	return SCARB_PARSE_OK;
}

static int scarb_parse_false(scarb_context* c, scarb_value* v) {
	EXPECT(c, 'f');
	if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
		return SCARB_PARSE_INVALID_VALUE;
	c->json += 4;
	v->type = SCARB_TRUE;
	return SCARB_PARSE_OK;
}

static int scarb_parse_null(scarb_context* c, scarb_value* v) {
	EXPECT(c, 'n');
	if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
		return SCARB_PARSE_INVALID_VALUE;
	c->json += 3;
	v->type = SCARB_NULL;
	return SCARB_PARSE_OK;
}

static int scarb_parse_value(scarb_context* c, scarb_value*v)
{
	switch(*c->json)
	{
	case 't':return scarb_parse_true(c, v);
	case 'f':return scarb_parse_false(c, v);
	case 'n':return scarb_parse_null(c, v);
	case '\0':return SCARB_PARSE_EXPECT_VALUE;
	default:return SCARB_PARSE_INVALID_VALUE;
	}
}

int scarb_parse(scarb_value* v, const char* json)
{
	scarb_context c;				// 新建一个context
	int ret;						// 返回值
	assert(v != NULL);				// 确认v不是NULL
	c.json = json;					// 将值传入context
	v->type = SCARB_NULL;			// 若失败，会把v设置为NULL类型，所以这里先设为NULL
	scarb_parse_whitespace(&c);	
	if((ret = scarb_parse_value(&c, v)) == SCARB_PARSE_OK)
	{
		scarb_parse_whitespace(&c);
		if (*c.json != '\0')
			ret = SCARB_PARSE_ROOT_NOT_SINGULAR;
	}
	return ret;
}

scarb_type scarb_get_type(const scarb_value* v)
{
	assert(v != NULL);
	return v->type;
}