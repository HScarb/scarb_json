#include "scarbjson.h"
#include <assert.h>		// assert()
#include <errno.h>		// errno, ERANGE
#include <math.h>		// HUGE_VAL
#include <stdlib.h>		// NULL,strtod

// *c->json != ch, report error, else c->json++
#define EXPECT(c, ch)		do{ assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)			((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)		((ch) >= '1' && (ch) <= '9')

/* 为了减少解析函数之间传递多个参数,把这些数据都放进一个结构体 */
typedef struct
{
	const char * json;
}scarb_context;

/*
	解析空白，
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
	v->type = SCARB_FALSE;
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

/** 解析文字，包括 true/false/null
*	c为要解析的文字
*	v为解析的结果值
*	literal为要解析的期望文字
*	type为解析成功赋给值的类型
*/
static int scarb_parse_literal(scarb_context* c, scarb_value* v, const char* literal, scarb_type type)
{
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 1; literal[i + 1]; i++)
	{
		if (c->json[i] != literal[i + 1])
			return SCARB_PARSE_INVALID_VALUE;
	}
	c->json += i;
	v->type = type;
	return SCARB_PARSE_OK;
}

/* 解析数 */
static int scarb_parse_number(scarb_context* c, scarb_value* v)
{
	const char* p = c->json;		// 表示当前的解析字符位置
/* 校验数字,排除json不允许的格式 */
	/* 负号 */
	if (*p == '-') p++;
	/* 整数 */
	if (*p == '0') p++;				// 跳过单个0
	else
	{
		if (!ISDIGIT1TO9(*p)) return SCARB_PARSE_INVALID_VALUE;		// 第一个数必定是1到9，否则不合法
		for (p++; ISDIGIT(*p); p++);// 跳过剩余digit
	}
	/* 小数 */
	if(*p == '.')
	{
		p++;
		if (!ISDIGIT(*p)) return SCARB_PARSE_INVALID_VALUE;			// 检测是否至少有一digit，没有就返回错误
		for (p++; ISDIGIT(*p); p++);								// 跳过剩余digit
	}
	/* 指数 */
	if(*p == 'e' || *p == 'E')
	{
		p++;														// 跳过e
		if (*p == '+' || *p == '-') p++;							// 如果有一个正或负号，跳过
		if (!ISDIGIT(*p))return SCARB_PARSE_INVALID_VALUE;			// 然后和小数的逻辑一样
		for (p++; ISDIGIT(*p); p++);
	}
	errno = 0;
	v->n = strtod(c->json, NULL);
	/* 如果数字过大，返回NUMBER_TOO_BIG */
	if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
		return SCARB_PARSE_NUMBER_TOO_BIG;
	v->type = SCARB_NUMBER;
	c->json = p;
	return SCARB_PARSE_OK;
}

static int scarb_parse_value(scarb_context* c, scarb_value*v)
{
	switch(*c->json)
	{
	case 't':return scarb_parse_literal(c, v, "true", SCARB_TRUE);
	case 'f':return scarb_parse_literal(c, v, "false", SCARB_FALSE);
	case 'n':return scarb_parse_literal(c, v, "null", SCARB_NULL);
	default:return scarb_parse_number(c, v);
	case '\0':return SCARB_PARSE_EXPECT_VALUE;
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
		{
			v->type = SCARB_NULL;
			ret = SCARB_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	return ret;
}

/** getter **/
scarb_type scarb_get_type(const scarb_value* v)
{
	assert(v != NULL);
	return v->type;
}

double scarb_get_number(const scarb_value* v)
{
	assert(v != NULL && v->type == SCARB_NUMBER);		// 仅当type==NULBER时，n才表示json数字的数值
	return v->n;
}