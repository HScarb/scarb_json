#pragma once

typedef enum { SCARB_NULL, SCARB_FALSE, SCARB_TRUE, SCARB_NUMBER, SCARB_STRING, SCARB_ARRAY, SCARB_OBJECT} scarb_type;

// JSON的数据结构
typedef struct
{
	scarb_type type;
}scarb_value;

/** 解析JSON
*	用法如下
*	lept_value v;
*	const char json[] = ...;
*	int ret = lept_parse(&v, json);
*/
int scarb_parse(scarb_value* v, char * json);

/*
	本单元JSON语法子集
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
	SCARB_PARSE_ROOT_NOT_SINGULAR		// 一个值之后，在空白之后还有其他字符
};

// 访问结果的函数，获取其类型
scarb_type scarb_get_type(const scarb_value* v);

