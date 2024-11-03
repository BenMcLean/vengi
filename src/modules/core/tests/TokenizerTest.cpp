/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/StringUtil.h"
#include "core/tests/TestHelper.h"
#include "core/Tokenizer.h"

namespace core {

class TokenizerTest: public testing::Test {
};

TEST_F(TokenizerTest, testTokenizerNoSkipComment) {
	const core::String str = "http://foo.bar";
	core::TokenizerConfig cfg;
	cfg.skipComments = false;
	core::Tokenizer t(cfg, str.c_str(), str.size(), ";");
	ASSERT_EQ(1u, t.size()) << string::toString(t.tokens());
	EXPECT_STREQ(str.c_str(), t.tokens()[0].c_str()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerSkipComment) {
	const core::String str = "http://foo.bar";
	core::TokenizerConfig cfg;
	core::Tokenizer t(cfg, str.c_str(), str.size(), ";");
	ASSERT_EQ(1u, t.size()) << toString(t.tokens());
	EXPECT_STREQ("http:", t.tokens()[0].c_str()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerEmptyLengthExceedsString) {
	core::Tokenizer t("", 100, ";");
	EXPECT_EQ(0u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerLengthExceedsString) {
	core::Tokenizer t("abc;def", 100, ";");
	EXPECT_EQ(2u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerOnlyFirstMatch) {
	core::Tokenizer t("abc;def", 3, ";");
	EXPECT_EQ(1u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerPeekNext) {
	core::Tokenizer t("abc;def", 7, ";");
	ASSERT_EQ(2u, t.size()) << toString(t.tokens());
	EXPECT_EQ(t.peekNext(), "abc") << toString(t.tokens());
	EXPECT_EQ(t.peekNext(), "abc") << toString(t.tokens());
	EXPECT_TRUE(t.isNext("abc")) << toString(t.tokens());
	EXPECT_EQ(t.next(), "abc") << toString(t.tokens());
	EXPECT_EQ(t.peekNext(), "def") << toString(t.tokens());
	EXPECT_TRUE(t.isNext("def")) << toString(t.tokens());
}

TEST_F(TokenizerTest, testBase64JsonArray) {
	core::Tokenizer t("{\n\t[\t\"Zm9vYmFy\",\n \"Zm9vYmFy\"]\n}\n", " \t\n,:", "{}[]");
	EXPECT_EQ(6u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testInnerQuoteSplit) {
	core::Tokenizer t("[\"=y\"]", " ", "[]");
	EXPECT_EQ(3u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testSingleSplitToken) {
	core::Tokenizer t("{\n", " ", "{");
	EXPECT_EQ(1u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerInvalidFile) {
	const char *buf = "\x22\x50\xe2\xf6\xe2\x20\xac\x55\x22";
	core::Tokenizer t(buf, 9, "\n");
	EXPECT_EQ(0u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerSecondMatchButEmptyString) {
	core::Tokenizer t("abc;def", 4, ";");
	ASSERT_EQ(2u, t.size()) << toString(t.tokens());
	EXPECT_EQ(t.tokens()[0], "abc") << toString(t.tokens());
	EXPECT_EQ(t.tokens()[1], "") << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerSecondMatchButOnlyOneChar) {
	core::Tokenizer t("abc;def", 5, ";");
	ASSERT_EQ(2u, t.size()) << toString(t.tokens());
	EXPECT_EQ(t.tokens()[0], "abc") << toString(t.tokens());
	EXPECT_EQ(t.tokens()[1], "d") << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerEmpty) {
	core::Tokenizer t("", ";");
	EXPECT_FALSE(t.hasNext());
	EXPECT_EQ(0u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerOnlySep) {
	core::Tokenizer t(";", ";");
	EXPECT_EQ(2u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerSepAndSplit) {
	core::Tokenizer t("int main(void) { foo; }", ";", "(){}");
	EXPECT_EQ(8u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerStrings) {
	core::Tokenizer t(";2;3;", ";");
	ASSERT_EQ(4u, t.size()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("2", t.next()) << toString(t.tokens());
	EXPECT_EQ("3", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	ASSERT_FALSE(t.hasNext()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerQuotedSeparator) {
	core::Tokenizer t("1;\"2;\";3;4", ";");
	ASSERT_EQ(4u, t.size()) << toString(t.tokens());
	EXPECT_EQ("1", t.next()) << toString(t.tokens());
	EXPECT_EQ("2;", t.next()) << toString(t.tokens());
	EXPECT_EQ("3", t.next()) << toString(t.tokens());
	EXPECT_EQ("4", t.next()) << toString(t.tokens());
	EXPECT_FALSE(t.hasNext()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerQuotedSeparatorFollowedByEmpty) {
	core::Tokenizer t("1;\"2;\";;", ";");
	ASSERT_EQ(4u, t.size()) << toString(t.tokens());
	EXPECT_EQ("1", t.next()) << toString(t.tokens());
	EXPECT_EQ("2;", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_FALSE(t.hasNext()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerInner) {
	core::Tokenizer t("1 \"somecommand \\\"inner\\\"\" 3");
	EXPECT_EQ(3u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerKeyBindings) {
	core::Tokenizer t("w +foo\nalt+a \"somecommand +\"\nCTRL+s +bar\nSHIFT+d +xyz\n");
	EXPECT_EQ(8u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerKeyQuotedSeparator) {
	core::Tokenizer t("2 \"1(\" 3");
	ASSERT_EQ(3u, t.size()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerCommandChain) {
	core::Tokenizer t(";;;;testsemicolon \";\";;;;", ";");
	ASSERT_EQ(9u, t.size()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("testsemicolon ;", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
	EXPECT_EQ("", t.next()) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerSimple) {
	EXPECT_EQ(9u, core::Tokenizer("some nice string that is easy to be tokenized").size());
	ASSERT_EQ(3u, core::Tokenizer("foo()").size());
	EXPECT_EQ(5u, core::Tokenizer("a +foo\nb+bar\nc +foobar").size());
	EXPECT_EQ(1u, core::Tokenizer("\"somecommand +\"").size());
	EXPECT_EQ(2u, core::Tokenizer("\"somecommand +\" \"somecommand +\"").size());
	EXPECT_EQ(1u, core::Tokenizer("\"somecommand \\\"inner\\\"\"").size());
	EXPECT_EQ(5u, core::Tokenizer("()()").size());
	EXPECT_EQ(4u, core::Tokenizer("1;2;3;4", ";").size());
	EXPECT_EQ(4u, core::Tokenizer("1;2;3;", ";").size());
	EXPECT_EQ(4u, core::Tokenizer(";2;3;", ";").size());
	EXPECT_EQ(4u, core::Tokenizer(";;;", ";").size());
	EXPECT_EQ(0u, core::Tokenizer("", ";").size());
	EXPECT_EQ(1u, core::Tokenizer("foo", ";").size());
	EXPECT_EQ(0u, core::Tokenizer("\n").size());
	EXPECT_EQ(5u, core::Tokenizer("{}{}").size());
	EXPECT_EQ(5u, core::Tokenizer("(){}").size());
	EXPECT_EQ(0u, core::Tokenizer("// empty").size());
	ASSERT_EQ(1u, core::Tokenizer("// empty\none").size());
	EXPECT_EQ(0u, core::Tokenizer("/* empty\none */").size());
	EXPECT_EQ(1u, core::Tokenizer("/* empty\none */\nfoo").size());
	ASSERT_EQ(2u, core::Tokenizer("one// empty\ntwo").size());
	EXPECT_EQ(1u, core::Tokenizer("one/* empty\ntwo */").size());
	EXPECT_EQ(2u, core::Tokenizer("one /* empty\ntwo */\nfoo").size());
	ASSERT_EQ(1u, core::Tokenizer("\"1()\"").size());
	ASSERT_EQ(2u, core::Tokenizer("2 \"1\"").size());
	ASSERT_EQ(4u, core::Tokenizer("2 \"1\" 3 \"4()\"").size());
	ASSERT_EQ(3u, core::Tokenizer("2 \"1()\" \"3\"").size());

	EXPECT_EQ("1()", core::Tokenizer("\"1()\"").next());
	EXPECT_EQ("foo", core::Tokenizer("foo()").next());
	EXPECT_EQ("foo", core::Tokenizer("foo\n").next());
	EXPECT_EQ("foo", core::Tokenizer("\nfoo\n").next());
	EXPECT_EQ("one", core::Tokenizer("// empty\none").next());
	EXPECT_EQ("one", core::Tokenizer("one// empty\ntwo").next());
	EXPECT_EQ("foo", core::Tokenizer("/* empty\none */\nfoo").next());
	EXPECT_EQ("bar", core::Tokenizer("/* empty\none */\n// foo\n bar").next());
}

TEST_F(TokenizerTest, testTokenizerSplit) {
	core::Tokenizer t("typedef struct f[4] vec3;", " ", ";");
	ASSERT_EQ(6u, t.tokens().size()) << toString(t.tokens());
	EXPECT_EQ(6u, t.size()) << toString(t.tokens());
	EXPECT_EQ(";", t.tokens()[4]) << toString(t.tokens());
	EXPECT_EQ("typedef", t.tokens()[0]) << toString(t.tokens());
}

TEST_F(TokenizerTest, testTokenizerSplit2) {
	// separator and split char followed by each other...
	core::Tokenizer t("foo bar {\n\tkey value\n}\n\nfoo2 bar2 {\n\t(key2 value2) {}\n}\n", " \t\n", "(){},;");
	ASSERT_EQ(17u, t.tokens().size()) << toString(t.tokens());
	EXPECT_EQ(17u, t.size()) << toString(t.tokens());
}

}
