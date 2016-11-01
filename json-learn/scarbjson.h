#pragma once

typedef enum { SCARB_NULL, SCARB_FALSE, SCARB_TRUE, SCARB_NUMBER, SCARB_STRING, SCARB_ARRAY, SCARB_OBJECT} scarb_type;

// JSON�����ݽṹ
typedef struct
{
	scarb_type type;
}scarb_value;

/** ����JSON
*	�÷�����
*	lept_value v;
*	const char json[] = ...;
*	int ret = lept_parse(&v, json);
*/
int scarb_parse(scarb_value* v, char * json);

/*
	����ԪJSON�﷨�Ӽ�
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
	SCARB_PARSE_ROOT_NOT_SINGULAR		// һ��ֵ֮���ڿհ�֮���������ַ�
};

// ���ʽ���ĺ�������ȡ������
scarb_type scarb_get_type(const scarb_value* v);

