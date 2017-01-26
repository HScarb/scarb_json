#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC		// 在 Windows 下，可使用 Visual C++ 的 CRT 检测内存泄漏
#include <stdlib.h>
#include <crtdbg.h>
#endif
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
			fprintf(stderr, "%s: line %d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
			main_ret = 1;\
		}\
	} while(0)

// 每次使用这个宏时，如果 expect != actual（预期值不等于实际值），便会输出错误信息。
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
	EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

static void test_parse_null()
{
	scarb_value v;
	scarb_init(&v);
	scarb_set_boolean(&v, 0);
	EXPECT_EQ_INT(SCARB_PARSE_OK, scarb_parse(&v, "null"));
	EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));
	scarb_free(&v);
}


static void test_parse_true() {
	scarb_value v;
	scarb_init(&v);
	scarb_set_boolean(&v, 0);
	EXPECT_EQ_INT(SCARB_PARSE_OK, scarb_parse(&v, "true"));
	EXPECT_EQ_INT(SCARB_TRUE, scarb_get_type(&v));
	scarb_free(&v);
}

static void test_parse_false() {
	scarb_value v;
	scarb_init(&v);
	scarb_set_boolean(&v, 1);
	EXPECT_EQ_INT(SCARB_PARSE_OK, scarb_parse(&v, "false"));
	EXPECT_EQ_INT(SCARB_FALSE, scarb_get_type(&v));
	scarb_free(&v);
}

/* 测试数值的宏，expect为测试期望值，json为测试的json值 */
#define TEST_NUMBER(expect, json)\
	do{\
		scarb_value v;\
		scarb_init(&v);\
		EXPECT_EQ_INT(SCARB_PARSE_OK, scarb_parse(&v, json));/*解析成scarb_value*/\
		EXPECT_EQ_INT(SCARB_NUMBER, scarb_get_type(&v));/*判断类型是否为number*/\
		EXPECT_EQ_DOUBLE(expect, scarb_get_number(&v));/*测试number值*/\
		scarb_free(&v);\
	} while(0)

static void test_parse_number() {
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

	/* 边界值测试 */
	TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
	TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
	do{\
		scarb_value v;\
		scarb_init(&v);\
		EXPECT_EQ_INT(SCARB_PARSE_OK, scarb_parse(&v, json));\
		EXPECT_EQ_INT(SCARB_STRING, scarb_get_type(&v));\
		EXPECT_EQ_STRING(expect, scarb_get_string(&v), scarb_get_string_length(&v));\
		scarb_free(&v);\
	} while(0)

static void test_parse_string()
{
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");
#if 0
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
}

/* 测试相应错误类型的宏，error为要测试的错误，json为测试的json值 */
#define TEST_ERROR(error, json)\
	do{\
		scarb_value v;\
		scarb_init(&v);\
		scarb_set_boolean(&v, 0);\
		EXPECT_EQ_INT(error, scarb_parse(&v, json));\
		EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));\
		scarb_free(&v);\
	} while(0)

/* 测试解析只含空白的值，返回PARSE_EXPECT_VALUE */
static void test_parse_expect_value() {
	TEST_ERROR(SCARB_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(SCARB_PARSE_EXPECT_VALUE, " ");
}

/* 测试解析无效的值，返回PARSE_INVALID_VALUE */
static void test_parse_invalid_value() {
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "?");

	/* invalid number */
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(SCARB_PARSE_INVALID_VALUE, "nan");
}

/* 测试解析，返回ROOT_NOT_SINGULAR错误 */
static void test_parse_root_not_singular() {
	TEST_ERROR(SCARB_PARSE_ROOT_NOT_SINGULAR, "null x");

	/* invalid number */
	TEST_ERROR(SCARB_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
	TEST_ERROR(SCARB_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(SCARB_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big()
{
	TEST_ERROR(SCARB_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(SCARB_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark()
{
	TEST_ERROR(SCARB_PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(SCARB_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() 
{
	TEST_ERROR(SCARB_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
	TEST_ERROR(SCARB_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
	TEST_ERROR(SCARB_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
	TEST_ERROR(SCARB_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
	TEST_ERROR(SCARB_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(SCARB_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_access_null()
{
	scarb_value v;
	scarb_init(&v);
	scarb_set_string(&v, "a", 1);
	scarb_set_null(&v);
	EXPECT_EQ_INT(SCARB_NULL, scarb_get_type(&v));
	scarb_free(&v);
}

static void test_access_boolean()
{
	scarb_value v;
	scarb_init(&v);
	scarb_set_string(&v, "a", 1);
	scarb_set_boolean(&v, 1);
	EXPECT_TRUE(scarb_get_boolean(&v));
	scarb_set_boolean(&v, 0);
	EXPECT_FALSE(scarb_get_boolean(&v));
	scarb_free(&v);
}

static void test_access_number()
{
	scarb_value v;
	scarb_init(&v);
	scarb_set_string(&v, "a", 1);
	scarb_set_number(&v, 1234.5);
	EXPECT_EQ_DOUBLE(1234.5, scarb_get_number(&v));
	scarb_free(&v);
}

static void test_access_string()
{
	scarb_value v;
	scarb_init(&v);
	scarb_set_string(&v, "", 0);
	EXPECT_EQ_STRING("", scarb_get_string(&v), scarb_get_string_length(&v));
	scarb_set_string(&v, "Hello", 5);
	EXPECT_EQ_STRING("Hello", scarb_get_string(&v), scarb_get_string_length(&v));
	scarb_free(&v);
}

static void test_parse()
{
	test_parse_null();
	test_parse_true();
	test_parse_false();
	test_parse_number();
	test_parse_string();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
	test_parse_number_too_big();
	test_parse_missing_quotation_mark();
	test_parse_invalid_string_escape();
	test_parse_invalid_string_char();

	test_access_null();
	test_access_boolean();
	test_access_number();
	test_access_string();
}

int main()
{
#ifdef _WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	system("pause");
	return main_ret;
}