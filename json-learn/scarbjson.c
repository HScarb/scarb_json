#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC		// 在 Windows 下，可使用 Visual C++ 的 CRT 检测内存泄漏
#include <crtdbg.h>
#endif
#include "scarbjson.h"
#include <assert.h>		// assert()
#include <errno.h>		// errno, ERANGE
#include <math.h>		// HUGE_VAL
#include <stdlib.h>		// NULL,strtod
#include <string.h>		// memcpy()

#ifndef SCARB_PARSE_STACK_INIT_SIZE
#define SCARB_PARSE_STACK_INIT_SIZE 256
#endif					// 使用者可在编译选项中自行设置宏，没设置的话就用缺省值。

// *c->json != ch, report error, else c->json++
#define EXPECT(c, ch)		do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)			((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)		((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)			do { *(char*)scarb_context_push(c, sizeof(char)) = (ch); } while(0)

/* 为了减少解析函数之间传递多个参数,把这些数据都放进一个结构体 */
typedef struct
{
	const char * json;
	// 动态堆栈
	char * stack;
	size_t size, top;
}scarb_context;

// 动态堆栈操作
static void* scarb_context_push(scarb_context* c, size_t size)
{
	void *ret;
	assert(size > 0);
	if(c->top + size >= c->size)			// 压入时空间不足
	{
		if (c->size == 0)
			c->size = SCARB_PARSE_STACK_INIT_SIZE;
		while (c->top + size >= c->size)	// 回以1.5倍大小扩展
			c->size += c->size >> 1;		// c->size * 1.5
		c->stack = (char*)realloc(c->stack, c->size);	// 用realloc重新分配内存 realloc(NULL, size) = malloc(size)
	}
	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

static void* scarb_context_pop(scarb_context* c, size_t size)
{
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

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

static int scarb_parse_string(scarb_context* c, scarb_value* v)
{
	size_t head = c->top, len;
	const char* p;
	EXPECT(c, '\"');						// 匹配第一个"
	p = c->json;
	for (;;)
	{
		char ch = *p++;
		switch(ch)
		{
		case'\"':							// 匹配第二个"
			len = c->top - head;
			scarb_set_string(v, (const char*)scarb_context_pop(c, len), len);
			c->json = p;
			return SCARB_PARSE_OK;
		case '\\':
			switch (*p++)
			{
			case '\"': PUTC(c, '\"'); break;
			case '\\': PUTC(c, '\\'); break;
			case '/': PUTC(c, '/'); break;
			case 'b': PUTC(c, '\b'); break;
			case 'f': PUTC(c, '\f'); break;
			case 'n': PUTC(c, '\n'); break;
			case 'r': PUTC(c, '\r'); break;
			case 't': PUTC(c, '\t'); break;
			default:
				c->top = head;
				return SCARB_PARSE_INVALID_STRING_ESCAPE;
			}
			break;
		case '\0':							// 匹配字符串结尾字符
			c->top = head;
			return SCARB_PARSE_MISS_QUOTATION_MARK;
		default:
			if((unsigned char)ch < 0x20)	// 处理不合法的字符串
			{
				c->top = head;
				return SCARB_PARSE_INVALID_STRING_CHAR;
			}
			PUTC(c, ch);
		}
	}
}

static int scarb_parse_value(scarb_context* c, scarb_value* v)
{
	switch(*c->json)
	{
	case 't':	return scarb_parse_literal(c, v, "true", SCARB_TRUE);
	case 'f':	return scarb_parse_literal(c, v, "false", SCARB_FALSE);
	case 'n':	return scarb_parse_literal(c, v, "null", SCARB_NULL);
	default:	return scarb_parse_number(c, v);
	case '"':	return scarb_parse_string(c, v);
	case '\0':	return SCARB_PARSE_EXPECT_VALUE;
	}
}

int scarb_parse(scarb_value* v, const char* json)
{
	scarb_context c;				// 新建一个context
	int ret;						// 返回值
	assert(v != NULL);				// 确认v不是NULL
	c.json = json;					// 将值传入context
	c.stack = NULL;
	c.size = c.top = 0;
	scarb_init(v);					// 若失败，会把v设置为NULL类型，所以这里先设为NULL
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
	assert(c.top == 0);				// 释放时 确保所有数据都被弹出
	free(c.stack);					// 释放栈
	return ret;
}

void scarb_free(scarb_value* v)
{
	assert(v != NULL);
	if (v->type == SCARB_STRING)
		free(v->s.s);
	v->type = SCARB_NULL;
}

/** getter seeter **/
scarb_type scarb_get_type(const scarb_value* v)
{
	assert(v != NULL);
	return v->type;
}

int scarb_get_boolean(const scarb_value* v)
{
	assert(v != NULL && (v->type == SCARB_TRUE || v->type == SCARB_FALSE));
	return v->type == SCARB_TRUE;
}

void scarb_set_boolean(scarb_value* v, int b)
{
	scarb_free(v);
	v->type = b ? SCARB_TRUE : SCARB_FALSE;
}

double scarb_get_number(const scarb_value* v)
{
	assert(v != NULL && v->type == SCARB_NUMBER);		// 仅当type==NULBER时，n才表示json数字的数值
	return v->n;
}

void scarb_set_number(scarb_value* v, double n)
{
	scarb_free(v);
	v->n = n;
	v->type = SCARB_NUMBER;
}

const char* scarb_get_string(const scarb_value* v)
{
	assert(v != NULL && v->type == SCARB_STRING);
	return v->s.s;
}

size_t scarb_get_string_length(const scarb_value* v)
{
	assert(v != NULL && v->type == SCARB_STRING);
	return v->s.len;
}

void scarb_set_string(scarb_value* v, const char* s, size_t len)
{
	assert(v != NULL && (s != NULL || len == 0));		// 非空指针或者零长度的字符串均合法
	scarb_free(v);										// 设置之前先清空v可能分配到的内存
	v->s.s = (char*)malloc(len + 1);					// +1 : 结尾空字符
	memcpy(v->s.s, s, len);
	v->s.s[len] = '\0';
	v->s.len = len;
	v->type = SCARB_STRING;
}