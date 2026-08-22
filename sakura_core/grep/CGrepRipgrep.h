/*!	@file
	@brief Grepのripgrep(rg.exe)高速検索パス

	既存のGrepエンジン(CGrepAgent::DoGrepTree/DoGrepFile)はそのまま残し、
	rg.exeで代替可能な検索に限りrg.exeへ処理を委譲する。
	出力形式(SetGrepResult/OutputPathInfo)は既存のものをそのまま利用し、
	タグジャンプ互換の出力書式を再実装しない。
*/
/*
	SPDX-License-Identifier: Zlib
*/
#ifndef SAKURA_CGREPRIPGREP_3F6E8C7C_9C0E_4B7D_9F3C_2B2C9C9E7B21_H_
#define SAKURA_CGREPRIPGREP_3F6E8C7C_9C0E_4B7D_9F3C_2B2C9C9E7B21_H_
#pragma once

#include <string>
#include <vector>
#include "charset/charset.h"	//ECodeType

struct SGrepOption;
class CGrepEnumKeys;

namespace CGrepRipgrep {

/*!
	@brief 文字列がASCII文字(U+0000～U+007F)のみで構成されているか判定する

	SJIS・EUC-JP・UTF-8はいずれもASCII範囲のバイト列が共通であるため、
	検索パターンがASCIIのみであれば、ファイルの実際の文字コードが何であっても
	(自動判別であっても)rg.exeでの検索結果は既存エンジンと一致する。
	この性質を利用して、自動判別時でもASCII検索に限りrgを使用可能にする。

	@param[in] pszText	判定する文字列(nullptrはtrue扱い)
*/
[[nodiscard]] bool IsAsciiOnly( const wchar_t* pszText ) noexcept;

/*!
	@brief 現在の検索条件でripgrepを使用できるか判定する

	以下のいずれかに該当する場合はfalseを返し、呼び出し元は既存のGrepエンジンへ
	フォールバックすること。
	  - チェックボックスでrg使用がOFF
	  - Grep置換(rg.exeには置換機能が無い)
	  - 編集中バッファ(:HWND:...)の検索
	  - 標準出力モード
	  - 結果出力が「該当部分」または「否該当行」(rg.exeの素の出力からは
	    マッチ文字列の長さが取得できず、"該当行"出力のみ対応する)
	  - ファイル名検索(検索キーが空)
	  - 絶対パスでの除外指定
	  - 文字コードセットが自動判別かつ検索キーにASCII以外の文字を含む場合
	    (ASCIIのみならIsAsciiOnly()の理由により自動判別のままrgを使用できる)
	  - 文字コードセットが自動判別以外でrg.exeに対応するラベルが無い

	@param[in] sGrepOption		Grepオプション(bGrepUseRipgrepを含む)
	@param[in] bIsHandleGrep	編集中のウィンドウ(:HWND:...)を対象とした検索か
	@param[in] pszKey			検索キー(nullptrまたは空文字列=ファイル名検索)
	@param[in] cGrepEnumKeys	解析済みの検索対象・除外パターン
*/
[[nodiscard]] bool CanUseRipgrep(
	const SGrepOption&		sGrepOption,
	bool					bIsHandleGrep,
	const wchar_t*			pszKey,
	const CGrepEnumKeys&	cGrepEnumKeys
) noexcept;

/*!
	@brief 文字コードセットに対応するrg.exeの--encodingラベル(WHATWG Encoding仕様のラベル)を取得する
	@retval true	対応するラベルが見つかった(pStrLabelに格納)
	@retval false	rg.exeに直接対応するラベルが無い(自動判別を含む)
*/
[[nodiscard]] bool TryGetRipgrepEncodingLabel( ECodeType nGrepCharSet, std::wstring* pStrLabel ) noexcept;

/*!
	@brief 設定されたrg.exeのパスを解決する

	優先順位: (1)設定で明示指定されたパス (2)sakura.exeと同じフォルダー (3)PATH環境変数

	@param[in] pszConfiguredPath	共通設定で指定されたrg.exeのパス(空文字列可)
	@return 実行可能なrg.exeのフルパス。見つからない場合は空文字列
*/
[[nodiscard]] std::wstring ResolveRipgrepPath( const wchar_t* pszConfiguredPath );

/*!
	@brief 指定されたrg.exeを`--version`付きで実行し、バージョン文字列(1行目)を取得する

	共通設定ダイアログでパスの妥当性を利用者に示すためのもの。起動または応答が
	一定時間(3秒)内に完了しない場合は失敗として扱う。

	@retval true	バージョン文字列を取得できた(pStrVersionに格納)
	@retval false	実行できなかった、またはタイムアウトした
*/
[[nodiscard]] bool TryGetRipgrepVersion( const std::wstring& strExePath, std::wstring* pStrVersion );

/*!
	@brief GrepInfo/SSearchOption/SGrepOptionからrg.exeに渡す引数(exe名を除く)を組み立てる

	@param[in] pszKey			検索キー
	@param[in] cGrepEnumKeys	解析済みの検索対象・除外パターン
	@param[in] sSearchOption	検索オプション(正規表現/大小文字/単語単位)
	@param[in] sGrepOption		Grepオプション(文字コード/サブフォルダー/ファイル毎最初のみ 等)
	@param[in] vPaths			検索対象パス(絶対パス、末尾に'\\'を含まない)
*/
[[nodiscard]] std::wstring BuildRipgrepArguments(
	const wchar_t*					pszKey,
	const CGrepEnumKeys&			cGrepEnumKeys,
	const struct SSearchOption&	sSearchOption,
	const SGrepOption&				sGrepOption,
	const std::vector<std::wstring>& vPaths
);

//! rg.exeの1ヒット行(`path\0line:col:text` UTF-8)をデコードした結果
struct SRipgrepHit {
	std::wstring	strFullPath;	//!< ヒットしたファイルのフルパス(UTF-16)
	long long		nLine = 0;		//!< 行番号(1～)
	int				nColumn = 0;	//!< 桁番号(1～、UTF-16コードユニット単位)
	std::wstring	strLineText;	//!< 行の文字列(UTF-16、改行なし)
};

/*!
	@brief rg.exeの1ヒット行 "path\0line:col:text" (改行文字を含まないUTF-8バイト列)をパースする

	rg.exeは `--null --column -n --no-heading --color never` で起動される前提。
	--null によりパス末尾がNUL(0x00)で確定するため、Windowsのドライブレター(`C:`)の
	コロンと行番号のコロンが曖昧にならない。

	@param[in]  pszUtf8Line	1行分のUTF-8バイト列(改行文字を含まない)
	@param[in]  nLen			pszUtf8Lineの長さ(バイト数)
	@param[out] pHit			パース結果
	@retval true	パースに成功した
	@retval false	想定した書式ではなかった(空行・rgの警告出力等)
*/
[[nodiscard]] bool ParseRipgrepHitLine( const char* pszUtf8Line, size_t nLen, SRipgrepHit* pHit );

//! ComputeGrepPathPartsの結果
struct SGrepPathParts {
	std::wstring	strFolder;		//!< OutputPathInfoへ渡すpszFolder相当
	std::wstring	strRelPath;		//!< OutputPathInfo/SetGrepResultへ渡すpszRelPath相当
};

/*!
	@brief 絶対パスのヒットファイルから、既存の出力整形(OutputPathInfo/SetGrepResult)が
		期待するpszFolder/pszRelPathを計算する

	CGrepAgent::DoGrepTree が各ディレクトリー階層を再帰しながら pszFolder/pszRelPath を
	組み立てているのに対し、rg.exeは1回のプロセス起動でツリー全体をフラットに返す。
	そのため得られた絶対パスとベースフォルダーから同じ値を逆算する。

	@param[in] pszFullPath			ヒットしたファイルの絶対パス
	@param[in] pszBaseFolder		検索対象パス(絶対パス、末尾に'\\'を含まない)
	@param[in] bGrepSeparateFolder	SGrepOption::bGrepSeparateFolder
	@param[in] bGrepOutputBaseFolder	SGrepOption::bGrepOutputBaseFolder
*/
[[nodiscard]] SGrepPathParts ComputeGrepPathParts(
	const wchar_t*	pszFullPath,
	const wchar_t*	pszBaseFolder,
	bool			bGrepSeparateFolder,
	bool			bGrepOutputBaseFolder
);

/*!
	@brief UTF-8バイト列中のバイトオフセットに対応するUTF-16コードユニット数を求める

	rg.exeの --column はUTF-8バイトオフセット(1始まり)である一方、
	サクラエディタのGrep結果の桁番号はUTF-16(wchar_t)インデックス(1始まり)であるため、
	サロゲートペアを含め変換が必要になる。

	@param[in] pszUtf8Text	UTF-8バイト列
	@param[in] nByteOffset	変換したいバイトオフセット(0開始、この手前までを変換する)
	@return pszUtf8Textの先頭からnByteOffsetバイトをデコードしたときのUTF-16コードユニット数
*/
[[nodiscard]] int ConvertUtf8ByteOffsetToUtf16Length( const char* pszUtf8Text, size_t nByteOffset );

} // namespace CGrepRipgrep
#endif /* SAKURA_CGREPRIPGREP_3F6E8C7C_9C0E_4B7D_9F3C_2B2C9C9E7B21_H_ */
