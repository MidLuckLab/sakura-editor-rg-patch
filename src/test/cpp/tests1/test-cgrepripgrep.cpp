/*! @file */
/*
	SPDX-License-Identifier: Zlib
*/
#include "pch.h"

#include "grep/CGrepRipgrep.h"
#include "grep/CGrepEnumKeys.h"
#include "agent/CGrepAgent.h"
#include "_main/global.h"

namespace {

SGrepOption MakeUsableGrepOption()
{
	SGrepOption sOpt;
	sOpt.bGrepUseRipgrep = true;
	sOpt.bGrepReplace = false;
	sOpt.bGrepStdout = false;
	sOpt.nGrepOutputLineType = 1;
	sOpt.nGrepCharSet = CODE_UTF8;
	return sOpt;
}

} // namespace

//===========================================================================
// IsAsciiOnly
//===========================================================================

TEST(CGrepRipgrep, IsAsciiOnly_AsciiText)
{
	EXPECT_TRUE(CGrepRipgrep::IsAsciiOnly(L"specific"));
	EXPECT_TRUE(CGrepRipgrep::IsAsciiOnly(L"foo_bar123!?"));
	EXPECT_TRUE(CGrepRipgrep::IsAsciiOnly(L""));
	EXPECT_TRUE(CGrepRipgrep::IsAsciiOnly(nullptr));
}

TEST(CGrepRipgrep, IsAsciiOnly_NonAsciiText)
{
	EXPECT_FALSE(CGrepRipgrep::IsAsciiOnly(L"日本語"));
	EXPECT_FALSE(CGrepRipgrep::IsAsciiOnly(L"specificあ"));	// 末尾だけ非ASCII
	EXPECT_FALSE(CGrepRipgrep::IsAsciiOnly(L"｡"));			// 半角カナも非ASCII扱い
}

//===========================================================================
// TryGetRipgrepEncodingLabel
//===========================================================================

TEST(CGrepRipgrep, TryGetRipgrepEncodingLabel_Supported)
{
	std::wstring strLabel;

	ASSERT_TRUE(CGrepRipgrep::TryGetRipgrepEncodingLabel(CODE_SJIS, &strLabel));
	EXPECT_EQ(strLabel, L"shift_jis");

	ASSERT_TRUE(CGrepRipgrep::TryGetRipgrepEncodingLabel(CODE_UTF8, &strLabel));
	EXPECT_EQ(strLabel, L"utf-8");

	ASSERT_TRUE(CGrepRipgrep::TryGetRipgrepEncodingLabel(CODE_EUC, &strLabel));
	EXPECT_EQ(strLabel, L"euc-jp");

	ASSERT_TRUE(CGrepRipgrep::TryGetRipgrepEncodingLabel(CODE_UNICODE, &strLabel));
	EXPECT_EQ(strLabel, L"utf-16le");

	ASSERT_TRUE(CGrepRipgrep::TryGetRipgrepEncodingLabel(CODE_UNICODEBE, &strLabel));
	EXPECT_EQ(strLabel, L"utf-16be");
}

TEST(CGrepRipgrep, TryGetRipgrepEncodingLabel_AutoDetectIsUnsupported)
{
	std::wstring strLabel;
	// 自動判別はrg.exeのBOMのみの判定と食い違うため、意図的に非対応とする
	EXPECT_FALSE(CGrepRipgrep::TryGetRipgrepEncodingLabel(CODE_AUTODETECT, &strLabel));
}

TEST(CGrepRipgrep, TryGetRipgrepEncodingLabel_Utf7IsUnsupported)
{
	std::wstring strLabel;
	EXPECT_FALSE(CGrepRipgrep::TryGetRipgrepEncodingLabel(CODE_UTF7, &strLabel));
}

//===========================================================================
// CanUseRipgrep
//===========================================================================

TEST(CGrepRipgrep, CanUseRipgrep_UsableCase)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	EXPECT_TRUE(CGrepRipgrep::CanUseRipgrep(MakeUsableGrepOption(), false, L"key", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_DisabledByCheckbox)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.bGrepUseRipgrep = false;
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(sOpt, false, L"key", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_GrepReplaceIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.bGrepReplace = true;
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(sOpt, false, L"key", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_HandleGrepIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	// 編集中バッファ(:HWND:...)の検索
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(MakeUsableGrepOption(), true, L"key", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_StdoutModeIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.bGrepStdout = true;
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(sOpt, false, L"key", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_MatchedPartOutputIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.nGrepOutputLineType = 0;	// 該当部分
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(sOpt, false, L"key", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_NonMatchingLineOutputIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.nGrepOutputLineType = 2;	// 否該当行
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(sOpt, false, L"key", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_FileNameSearchIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	// 検索キーが空(ファイル名検索)
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(MakeUsableGrepOption(), false, L"", cKeys));
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(MakeUsableGrepOption(), false, nullptr, cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_AutoDetectCharsetWithNonAsciiKeyIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.nGrepCharSet = CODE_AUTODETECT;
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(sOpt, false, L"日本語", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_AutoDetectCharsetWithAsciiKeyIsSupported)
{
	// SJIS/EUC-JP/UTF-8はASCII範囲が共通のため、検索キーがASCIIのみなら
	// 文字コードセット自動判別のままでもrgを使用できる
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.nGrepCharSet = CODE_AUTODETECT;
	EXPECT_TRUE(CGrepRipgrep::CanUseRipgrep(sOpt, false, L"specific", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_AbsoluteExcludeFileIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"!C:\\secret\\*.log");
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(MakeUsableGrepOption(), false, L"key", cKeys));
}

TEST(CGrepRipgrep, CanUseRipgrep_AbsoluteExcludeFolderIsUnsupported)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"#C:\\secret");
	EXPECT_FALSE(CGrepRipgrep::CanUseRipgrep(MakeUsableGrepOption(), false, L"key", cKeys));
}

//===========================================================================
// BuildRipgrepArguments
//===========================================================================

TEST(CGrepRipgrep, BuildRipgrepArguments_RegularExpAndCaseSensitive)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.cpp;*.h");
	SSearchOption sSearch;
	sSearch.bRegularExp = true;
	sSearch.bLoHiCase = true;
	sSearch.bWordOnly = false;
	SGrepOption sOpt = MakeUsableGrepOption();

	std::wstring strArgs = CGrepRipgrep::BuildRipgrepArguments(
		L"pattern", cKeys, sSearch, sOpt, { L"C:\\work" });

	EXPECT_NE(strArgs.find(L"--null"), std::wstring::npos);
	EXPECT_NE(strArgs.find(L"-e pattern"), std::wstring::npos);
	EXPECT_NE(strArgs.find(L"--glob=*.cpp"), std::wstring::npos);
	EXPECT_NE(strArgs.find(L"--glob=*.h"), std::wstring::npos);
	EXPECT_NE(strArgs.find(L"C:\\work"), std::wstring::npos);
	// 正規表現・大小文字区別が有効な場合は -F も -i も付かない
	// (空白区切りのトークンとして探す。"--no-ignore" 等の固定フラグ文字列の
	//  部分文字列としての誤検出を避けるため単純なfind()は使わない)
	EXPECT_EQ(strArgs.find(L" -F "), std::wstring::npos);
	EXPECT_EQ(strArgs.find(L" -i "), std::wstring::npos);
}

TEST(CGrepRipgrep, BuildRipgrepArguments_FixedStringAndIgnoreCaseAndWordOnly)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SSearchOption sSearch;
	sSearch.bRegularExp = false;
	sSearch.bLoHiCase = false;
	sSearch.bWordOnly = true;
	SGrepOption sOpt = MakeUsableGrepOption();

	std::wstring strArgs = CGrepRipgrep::BuildRipgrepArguments(
		L"key", cKeys, sSearch, sOpt, { L"C:\\work" });

	EXPECT_NE(strArgs.find(L"-F"), std::wstring::npos);
	EXPECT_NE(strArgs.find(L"-i"), std::wstring::npos);
	EXPECT_NE(strArgs.find(L"-w"), std::wstring::npos);
}

TEST(CGrepRipgrep, BuildRipgrepArguments_NoSubFolderAddsMaxDepth1)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SSearchOption sSearch;
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.bGrepSubFolder = false;

	std::wstring strArgs = CGrepRipgrep::BuildRipgrepArguments(
		L"key", cKeys, sSearch, sOpt, { L"C:\\work" });

	EXPECT_NE(strArgs.find(L"--max-depth=1"), std::wstring::npos);
}

TEST(CGrepRipgrep, BuildRipgrepArguments_FileOnlyAddsMaxCount1)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*");
	SSearchOption sSearch;
	SGrepOption sOpt = MakeUsableGrepOption();
	sOpt.bGrepOutputFileOnly = true;

	std::wstring strArgs = CGrepRipgrep::BuildRipgrepArguments(
		L"key", cKeys, sSearch, sOpt, { L"C:\\work" });

	EXPECT_NE(strArgs.find(L"--max-count=1"), std::wstring::npos);
}

TEST(CGrepRipgrep, BuildRipgrepArguments_ExcludePatternsAreNegatedGlobs)
{
	CGrepEnumKeys cKeys;
	cKeys.SetFileKeys(L"*.*;!*.obj;#.git");
	SSearchOption sSearch;
	SGrepOption sOpt = MakeUsableGrepOption();

	std::wstring strArgs = CGrepRipgrep::BuildRipgrepArguments(
		L"key", cKeys, sSearch, sOpt, { L"C:\\work" });

	EXPECT_NE(strArgs.find(L"--glob=!*.obj"), std::wstring::npos);
	EXPECT_NE(strArgs.find(L"--glob=!.git"), std::wstring::npos);
}

//===========================================================================
// ParseRipgrepHitLine
//===========================================================================

TEST(CGrepRipgrep, ParseRipgrepHitLine_ValidLine)
{
	const std::string strLine = std::string("C:\\work\\file.cpp") + '\0' + "12:5:hello world";
	CGrepRipgrep::SRipgrepHit hit;
	ASSERT_TRUE(CGrepRipgrep::ParseRipgrepHitLine(strLine.data(), strLine.size(), &hit));
	EXPECT_EQ(hit.strFullPath, L"C:\\work\\file.cpp");
	EXPECT_EQ(hit.nLine, 12);
	EXPECT_EQ(hit.nColumn, 5);	// ASCIIのみの行なのでバイトオフセット=UTF-16インデックス
	EXPECT_EQ(hit.strLineText, L"hello world");
}

TEST(CGrepRipgrep, ParseRipgrepHitLine_NoNulIsRejected)
{
	const std::string strLine = "C:\\work\\file.cpp:12:5:hello world";
	CGrepRipgrep::SRipgrepHit hit;
	EXPECT_FALSE(CGrepRipgrep::ParseRipgrepHitLine(strLine.data(), strLine.size(), &hit));
}

TEST(CGrepRipgrep, ParseRipgrepHitLine_NonNumericLineIsRejected)
{
	const std::string strLine = std::string("C:\\work\\file.cpp") + '\0' + "abc:5:hello";
	CGrepRipgrep::SRipgrepHit hit;
	EXPECT_FALSE(CGrepRipgrep::ParseRipgrepHitLine(strLine.data(), strLine.size(), &hit));
}

TEST(CGrepRipgrep, ParseRipgrepHitLine_MissingColumnIsRejected)
{
	const std::string strLine = std::string("C:\\work\\file.cpp") + '\0' + "12:hello";
	CGrepRipgrep::SRipgrepHit hit;
	EXPECT_FALSE(CGrepRipgrep::ParseRipgrepHitLine(strLine.data(), strLine.size(), &hit));
}

TEST(CGrepRipgrep, ParseRipgrepHitLine_JapaneseTextColumnConversion)
{
	// "あいu" の後に一致(u=1バイト, あ・いはUTF-8で3バイトずつ=計6バイト → バイトオフセット7で一致開始)
	// バイト列: "あいu" (7バイト) + "hit"
	const std::string strPath = "C:\\work\\file.cpp";
	const std::string strText = "\xE3\x81\x82\xE3\x81\x84u" "hit";	// "あい" + "u" + "hit"
	const std::string strLine = strPath + '\0' + "1:8:" + strText;
	// バイトオフセット(1始まり)8 = "あいu"(7バイト)の次の文字から一致
	CGrepRipgrep::SRipgrepHit hit;
	ASSERT_TRUE(CGrepRipgrep::ParseRipgrepHitLine(strLine.data(), strLine.size(), &hit));
	EXPECT_EQ(hit.strLineText, L"\u3042\u3044uhit");	// "あいuhit"
	// UTF-16では "あいu" は3コードユニット、よって桁は 3+1=4
	EXPECT_EQ(hit.nColumn, 4);
}

//===========================================================================
// ConvertUtf8ByteOffsetToUtf16Length
//===========================================================================

TEST(CGrepRipgrep, ConvertUtf8ByteOffsetToUtf16Length_Ascii)
{
	const char* pszText = "hello";
	EXPECT_EQ(CGrepRipgrep::ConvertUtf8ByteOffsetToUtf16Length(pszText, 0), 0);
	EXPECT_EQ(CGrepRipgrep::ConvertUtf8ByteOffsetToUtf16Length(pszText, 3), 3);
}

TEST(CGrepRipgrep, ConvertUtf8ByteOffsetToUtf16Length_Japanese)
{
	// "あ" (U+3042) はUTF-8で3バイト、UTF-16で1コードユニット
	const char* pszText = "\xE3\x81\x82\xE3\x81\x84X";	// "あい" + "X"
	EXPECT_EQ(CGrepRipgrep::ConvertUtf8ByteOffsetToUtf16Length(pszText, 0), 0);
	EXPECT_EQ(CGrepRipgrep::ConvertUtf8ByteOffsetToUtf16Length(pszText, 3), 1);	// "あ"の直後
	EXPECT_EQ(CGrepRipgrep::ConvertUtf8ByteOffsetToUtf16Length(pszText, 6), 2);	// "あい"の直後
	EXPECT_EQ(CGrepRipgrep::ConvertUtf8ByteOffsetToUtf16Length(pszText, 7), 3);	// "あいX"の直後
}

TEST(CGrepRipgrep, ConvertUtf8ByteOffsetToUtf16Length_SurrogatePair)
{
	// U+1F600 (😀) はUTF-8で4バイト、UTF-16ではサロゲートペアで2コードユニット
	const char* pszText = "\xF0\x9F\x98\x80X";	// U+1F600 + "X"
	EXPECT_EQ(CGrepRipgrep::ConvertUtf8ByteOffsetToUtf16Length(pszText, 4), 2);
}

//===========================================================================
// ComputeGrepPathParts
//===========================================================================

TEST(CGrepRipgrep, ComputeGrepPathParts_Default)
{
	auto parts = CGrepRipgrep::ComputeGrepPathParts(
		L"C:\\work\\sub\\file.cpp", L"C:\\work", false, false);
	EXPECT_EQ(parts.strRelPath, L"sub\\file.cpp");
	EXPECT_EQ(parts.strFolder, L"C:\\work\\sub");
}

TEST(CGrepRipgrep, ComputeGrepPathParts_SeparateFolder)
{
	auto parts = CGrepRipgrep::ComputeGrepPathParts(
		L"C:\\work\\sub\\file.cpp", L"C:\\work", true, false);
	EXPECT_EQ(parts.strRelPath, L"file.cpp");
	EXPECT_EQ(parts.strFolder, L"C:\\work\\sub");
}

TEST(CGrepRipgrep, ComputeGrepPathParts_SeparateFolderAndBaseFolder)
{
	auto parts = CGrepRipgrep::ComputeGrepPathParts(
		L"C:\\work\\sub\\file.cpp", L"C:\\work", true, true);
	EXPECT_EQ(parts.strRelPath, L"file.cpp");
	EXPECT_EQ(parts.strFolder, L"sub");
}

TEST(CGrepRipgrep, ComputeGrepPathParts_FileDirectlyInBaseFolder)
{
	auto parts = CGrepRipgrep::ComputeGrepPathParts(
		L"C:\\work\\file.cpp", L"C:\\work", true, true);
	EXPECT_EQ(parts.strRelPath, L"file.cpp");
	EXPECT_EQ(parts.strFolder, L"");
}
