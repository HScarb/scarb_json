#include "scarbjson.h"
#include <assert.h>		// assert()
#include <stdlib.h>		// NULL

// c != ch, report error
#define EXPECT(c, ch)		do{ assert(*c->json == (ch)); c->json++; } while(0)

/* Ϊ�˼��ٽ�������֮�䴫�ݶ������,����Щ���ݶ��Ž�һ���ṹ�� */
typedef struct
{
	const char * json;
}scarb_context;

/*
	�����ո�
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
			ret = SCARB_PARSE_ROOT_NOT_SINGULAR;
	}
	return ret;
}

scarb_type scarb_get_type(const scarb_value* v)
{
	assert(v != NULL);
	return v->type;
}