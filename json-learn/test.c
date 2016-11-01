#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scarbjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
	do {\
		test_count++;\
		if(equality)\
			test_pass++;\
		else{\
			fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
			main_ret = 1;\
		}\
	} while(0)

// 每次使用这个宏时，如果 expect != actual（预期值不等于实际值），便会输出错误信息。
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

static void test_parse_null()
{
	scarb_value v;
	v.type = SCARB_TRUE;
	EXPECT_EQ_INT(SCARB_PARSE_OK, scarb_parse(&v, "null"));
	EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));
}


static void test_parse_true() {
	scarb_value v;
	v.type = SCARB_TRUE;
	EXPECT_EQ_INT(SCARB_PARSE_OK, scarb_parse(&v, "true"));
	EXPECT_EQ_INT(SCARB_TRUE, scarb_get_type(&v));
}

static void test_parse_false() {
	scarb_value v;
	v.type = SCARB_TRUE;
	EXPECT_EQ_INT(SCARB_PARSE_OK, scarb_parse(&v, "false"));
	EXPECT_EQ_INT(SCARB_FALSE, scarb_get_type(&v));
}

static void test_parse_expect_value() {
	scarb_value v;

	v.type = SCARB_TRUE;
	EXPECT_EQ_INT(SCARB_PARSE_EXPECT_VALUE, scarb_parse(&v, ""));
	EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));

	v.type = SCARB_TRUE;
	EXPECT_EQ_INT(SCARB_PARSE_EXPECT_VALUE, scarb_parse(&v, " "));
	EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));
}

static void test_parse_invalid_value() {
	scarb_value v;
	v.type = SCARB_TRUE;
	EXPECT_EQ_INT(SCARB_PARSE_INVALID_VALUE, scarb_parse(&v, "nul"));
	EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));

	v.type = SCARB_TRUE;
	EXPECT_EQ_INT(SCARB_PARSE_INVALID_VALUE, scarb_parse(&v, "?"));
	EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));
}

static void test_parse_root_not_singular() {
	scarb_value v;
	v.type = SCARB_TRUE;
	EXPECT_EQ_INT(SCARB_PARSE_ROOT_NOT_SINGULAR, scarb_parse(&v, "null x"));
	EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));
}

static void test_parse()
{
	test_parse_null();
	test_parse_true();
	test_parse_false();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
}

int main()
{
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	system("pause");
	return main_ret;
}