#pragma once

#include <stddef.h>		// size_t

typedef enum { SCARB_NULL, SCARB_FALSE, SCARB_TRUE, SCARB_NUMBER, SCARB_STRING, SCARB_ARRAY, SCARB_OBJECT} scarb_type;

// JSON的数据结构
typedef struct
{
	union
	{
		struct { char * s; size_t len; }s;		// json 字符串
		double n;			// json 数字
	};
	scarb_type type;		// 存储json数据类型
}scarb_value;

/** 解析JSON为一个scarb_value值
*	用法如下
*	lept_value v;
*	const char json[] = ...;
*	int ret = scarb_parse(&v, json);
*/
int scarb_parse(scarb_value* v, const char * json);

/*
	第一单元JSON语法子集
	JSON-text = ws value ws
	ws = *(%x20 / %x09 / %x0A / %x0D)
	value = null / false / true
	null  = "null"
	false = "false"
	true  = "true"
*/
// 解析JSON的 返回值
enum
{
	SCARB_PARSE_OK = 0,					// 无错误
	SCARB_PARSE_EXPECT_VALUE,			// JSON只含有空白
	SCARB_PARSE_INVALID_VALUE,			// JSON的value不是 null/false/true
	SCARB_PARSE_ROOT_NOT_SINGULAR,		// 一个值之后，在空白之后还有其他字符
	SCARB_PARSE_NUMBER_TOO_BIG,			// 数值过大
	SCARB_PARSE_MISS_QUOTATION_MARK,	// 字符串缺少引号
	SCARB_PARSE_INVALID_STRING_ESCAPE,	// 错误的转义字符
	SCARB_PARSE_INVALID_STRING_CHAR		// 不合法的字符串
};

#define scarb_init(v) do {(v)->type = SCARB_NULL; } while(0)

int scarb_parse(scarb_value * v, const char* json);

void scarb_free(scarb_value * v);

// 访问结果的函数，获取其类型
scarb_type scarb_get_type(const scarb_value* v);

#define scarb_set_null(v) scarb_free(v)

int scarb_get_boolean(const scarb_value* v);
void scarb_set_boolean(scarb_value* v, int b);

// 获取value的number值
double scarb_get_number(const scarb_value* v);
void scarb_set_number(scarb_value* v, double n);

const char * scarb_get_string(const scarb_value* v);
size_t scarb_get_string_length(const scarb_value* v);
void scarb_set_string(scarb_value* v, const char* s, size_t len);