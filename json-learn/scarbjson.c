#include "scarbjson.h"
#include <assert.h>		// assert()
#include <errno.h>		// errno, ERANGE
#include <math.h>		// HUGE_VAL
#include <stdlib.h>		// NULL,strtod

// *c->json != ch, report error, else c->json++
#define EXPECT(c, ch)		do{ assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)			((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)		((ch) >= '1' && (ch) <= '9')

/* Ϊ�˼��ٽ�������֮�䴫�ݶ������,����Щ���ݶ��Ž�һ���ṹ�� */
typedef struct
{
	const char * json;
}scarb_context;

/*
	�����հף�
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

/** �������֣����� true/false/null
*	cΪҪ����������
*	vΪ�����Ľ��ֵ
*	literalΪҪ��������������
*	typeΪ�����ɹ�����ֵ������
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

/* ������ */
static int scarb_parse_number(scarb_context* c, scarb_value* v)
{
	const char* p = c->json;		// ��ʾ��ǰ�Ľ����ַ�λ��
/* У������,�ų�json������ĸ�ʽ */
	/* ���� */
	if (*p == '-') p++;
	/* ���� */
	if (*p == '0') p++;				// ��������0
	else
	{
		if (!ISDIGIT1TO9(*p)) return SCARB_PARSE_INVALID_VALUE;		// ��һ�����ض���1��9�����򲻺Ϸ�
		for (p++; ISDIGIT(*p); p++);// ����ʣ��digit
	}
	/* С�� */
	if(*p == '.')
	{
		p++;
		if (!ISDIGIT(*p)) return SCARB_PARSE_INVALID_VALUE;			// ����Ƿ�������һdigit��û�оͷ��ش���
		for (p++; ISDIGIT(*p); p++);								// ����ʣ��digit
	}
	/* ָ�� */
	if(*p == 'e' || *p == 'E')
	{
		p++;														// ����e
		if (*p == '+' || *p == '-') p++;							// �����һ�����򸺺ţ�����
		if (!ISDIGIT(*p))return SCARB_PARSE_INVALID_VALUE;			// Ȼ���С�����߼�һ��
		for (p++; ISDIGIT(*p); p++);
	}
	errno = 0;
	v->n = strtod(c->json, NULL);
	/* ������ֹ��󣬷���NUMBER_TOO_BIG */
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
	scarb_context c;				// �½�һ��context
	int ret;						// ����ֵ
	assert(v != NULL);				// ȷ��v����NULL
	c.json = json;					// ��ֵ����context
	v->type = SCARB_NULL;			// ��ʧ�ܣ����v����ΪNULL���ͣ�������������ΪNULL
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
	assert(v != NULL && v->type == SCARB_NUMBER);		// ����type==NULBERʱ��n�ű�ʾjson���ֵ���ֵ
	return v->n;
}