#pragma once

#include <stddef.h>		// size_t

typedef enum { SCARB_NULL, SCARB_FALSE, SCARB_TRUE, SCARB_NUMBER, SCARB_STRING, SCARB_ARRAY, SCARB_OBJECT} scarb_type;

// JSON�����ݽṹ
typedef struct
{
	union
	{
		struct { char * s; size_t len; }s;		// json �ַ���
		double n;			// json ����
	};
	scarb_type type;		// �洢json��������
}scarb_value;

/** ����JSONΪһ��scarb_valueֵ
*	�÷�����
*	lept_value v;
*	const char json[] = ...;
*	int ret = scarb_parse(&v, json);
*/
int scarb_parse(scarb_value* v, const char * json);

/*
	��һ��ԪJSON�﷨�Ӽ�
	JSON-text = ws value ws
	ws = *(%x20 / %x09 / %x0A / %x0D)
	value = null / false / true
	null  = "null"
	false = "false"
	true  = "true"
*/
// ����JSON�� ����ֵ
enum
{
	SCARB_PARSE_OK = 0,					// �޴���
	SCARB_PARSE_EXPECT_VALUE,			// JSONֻ���пհ�
	SCARB_PARSE_INVALID_VALUE,			// JSON��value���� null/false/true
	SCARB_PARSE_ROOT_NOT_SINGULAR,		// һ��ֵ֮���ڿհ�֮���������ַ�
	SCARB_PARSE_NUMBER_TOO_BIG,			// ��ֵ����
	SCARB_PARSE_MISS_QUOTATION_MARK,	// �ַ���ȱ������
	SCARB_PARSE_INVALID_STRING_ESCAPE,	// �����ת���ַ�
	SCARB_PARSE_INVALID_STRING_CHAR		// ���Ϸ����ַ���
};

#define scarb_init(v) do {(v)->type = SCARB_NULL; } while(0)

int scarb_parse(scarb_value * v, const char* json);

void scarb_free(scarb_value * v);

// ���ʽ���ĺ�������ȡ������
scarb_type scarb_get_type(const scarb_value* v);

#define scarb_set_null(v) scarb_free(v)

int scarb_get_boolean(const scarb_value* v);
void scarb_set_boolean(scarb_value* v, int b);

// ��ȡvalue��numberֵ
double scarb_get_number(const scarb_value* v);
void scarb_set_number(scarb_value* v, double n);

const char * scarb_get_string(const scarb_value* v);
size_t scarb_get_string_length(const scarb_value* v);
void scarb_set_string(scarb_value* v, const char* s, size_t len);