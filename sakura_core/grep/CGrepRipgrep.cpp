/*!	@file
	@brief Grepのripgrep(rg.exe)高速検索パス

	@sa CGrepRipgrep.h
*/
/*
	SPDX-License-Identifier: Zlib
*/
#include "StdAfx.h"
#include "grep/CGrepRipgrep.h"

#include <algorithm>
#include <cctype>

#include "agent/CGrepAgent.h"
#include "grep/CGrepEnumKeys.h"
#include "_main/global.h"			//SSearchOption
#include "charset/CUtf8.h"
#include "mem/CMemory.h"
#include "mem/CNativeW.h"
#include "dlg/CDlgCancel.h"
#include "window/CEditWnd.h"
#include "util/file.h"				//GetExedir, IsFileExists, SplitPath_FolderAndFile
#include "util/window.h"			//BlockingHook
#include "apiwrap/StdApi.h"
#include "apiwrap/StdControl.h"
#include "sakura_rc.h"

namespace {

constexpr DWORD kUiCheckIntervalMillisec = 100;		// CGrepAgent.cpp の UICHECK_INTERVAL_MILLISEC と同じ値
constexpr DWORD kAddTailIntervalMillisec = 50;			// CGrepAgent.cpp の ADDTAIL_INTERVAL_MILLISEC と同じ値
constexpr DWORD kUiFileNameIntervalMillisec = 15;		// CGrepAgent.cpp の UIFILENAME_INTERVAL_MILLISEC と同じ値

/*!
	@brief コマンドライン引数1個を、必要であれば二重引用符で囲んでcmdlineに追加する

	CreateProcess に渡すコマンドラインの引数エスケープ規則(MSDNで説明されている、
	CommandLineToArgvW の逆変換に相当するアルゴリズム)に従う。
*/
void AppendQuotedArg( std::wstring& strCmdLine, const std::wstring& strArg )
{
	if( !strCmdLine.empty() ){
		strCmdLine += L' ';
	}
	if( !strArg.empty() && strArg.find_first_of( L" \t\n\v\"" ) == std::wstring::npos ){
		strCmdLine += strArg;
		return;
	}
	strCmdLine += L'"';
	for( size_t i = 0; i < strArg.size(); ){
		size_t nBackslashes = 0;
		while( i < strArg.size() && strArg[i] == L'\\' ){
			++nBackslashes;
			++i;
		}
		if( i == strArg.size() ){
			strCmdLine.append( nBackslashes * 2, L'\\' );
			break;
		}else if( strArg[i] == L'"' ){
			strCmdLine.append( nBackslashes * 2 + 1, L'\\' );
			strCmdLine += L'"';
			++i;
		}else{
			strCmdLine.append( nBackslashes, L'\\' );
			strCmdLine += strArg[i];
			++i;
		}
	}
	strCmdLine += L'"';
}

//! パイプから読み取ったUTF-8バイト列を行単位(LFまたはCRLF区切り)に分割する
class CLineSplitter {
public:
	template <class Handler>
	void Feed( const char* pData, size_t nLen, Handler&& handler )
	{
		m_strBuffer.append( pData, nLen );
		size_t nStart = 0;
		for( ;; ){
			size_t nPos = m_strBuffer.find( '\n', nStart );
			if( nPos == std::string::npos ){
				break;
			}
			size_t nEnd = nPos;
			if( nEnd > nStart && m_strBuffer[nEnd - 1] == '\r' ){
				--nEnd;
			}
			handler( m_strBuffer.data() + nStart, nEnd - nStart );
			nStart = nPos + 1;
		}
		m_strBuffer.erase( 0, nStart );
	}
private:
	std::string m_strBuffer;
};

/*!
	@brief rg.exeを起動し、標準出力を1行ずつhandlerへ渡す

	CEditView::ExecCmd() (view/CEditView_ExecCmd.cpp) と同様にパイプ経由で
	子プロセスの標準出力を読み取るが、以下の点で異なる簡易実装とする。
	  - Grep自身のCDlgCancelでキャンセルできるようにする(ExecCmd自身のIDD_EXECRUNNINGは使わない)
	  - 標準入力は使用しない
	  - 出力は常にUTF-8として扱う(rg.exeへ--no-configで渡すため)

	@retval true	最後まで実行できた(0件ヒットや起動失敗も含む)
	@retval false	ユーザーがキャンセルした
*/
template <class Handler>
bool RunRipgrepAndParseLines(
	const std::wstring&	strExePath,
	const std::wstring&	strArguments,
	const wchar_t*			pszWorkingDir,
	CDlgCancel*				pcDlgCancel,
	Handler&&				handler
)
{
	std::wstring strCmdLine;
	AppendQuotedArg( strCmdLine, strExePath );
	strCmdLine += L' ';
	strCmdLine += strArguments;

	SECURITY_ATTRIBUTES sa = { sizeof(sa) };
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = nullptr;

	HANDLE hReadPipe = nullptr;
	HANDLE hWritePipe = nullptr;
	if( !::CreatePipe( &hReadPipe, &hWritePipe, &sa, 0 ) ){
		return true;	// パイプを作れなかった。0件ヒットとして扱う
	}
	::SetHandleInformation( hReadPipe, HANDLE_FLAG_INHERIT, 0 );

	STARTUPINFOW sui = { sizeof(sui) };
	sui.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	sui.wShowWindow = SW_HIDE;
	sui.hStdInput = nullptr;
	sui.hStdOutput = hWritePipe;
	sui.hStdError = hWritePipe;

	PROCESS_INFORMATION pi{};
	std::vector<wchar_t> vCmdLineBuf( strCmdLine.begin(), strCmdLine.end() );
	vCmdLineBuf.push_back( L'\0' );

	BOOL bCreated = ::CreateProcessW(
		strExePath.c_str(),
		vCmdLineBuf.data(),
		nullptr, nullptr, TRUE,
		CREATE_NO_WINDOW,
		nullptr,
		pszWorkingDir,
		&sui, &pi
	);
	::CloseHandle( hWritePipe );
	if( !bCreated ){
		::CloseHandle( hReadPipe );
		return true;	// rg.exeを起動できなかった。0件ヒットとして扱う
	}

	CLineSplitter splitter;
	bool bCanceled = false;
	char work[8192];

	auto CheckCanceled = [&]() -> bool {
		if( pcDlgCancel && pcDlgCancel->IsCanceled() ){
			return true;
		}
		if( !::BlockingHook( pcDlgCancel ? pcDlgCancel->GetHwnd() : nullptr ) ){
			return true;
		}
		return false;
	};

	for( ;; ){
		DWORD dwWait = ::MsgWaitForMultipleObjects( 1, &pi.hProcess, FALSE, 20, QS_ALLEVENTS );
		if( dwWait == WAIT_OBJECT_0 + 1 ){
			if( CheckCanceled() ){
				::TerminateProcess( pi.hProcess, 0 );
				bCanceled = true;
				break;
			}
		}

		DWORD nAvail = 0;
		while( ::PeekNamedPipe( hReadPipe, nullptr, 0, nullptr, &nAvail, nullptr ) && 0 < nAvail ){
			DWORD nRead = 0;
			DWORD nToRead = (std::min)( nAvail, DWORD(sizeof(work)) );
			if( !::ReadFile( hReadPipe, work, nToRead, &nRead, nullptr ) || nRead == 0 ){
				break;
			}
			splitter.Feed( work, size_t(nRead), handler );

			if( CheckCanceled() ){
				::TerminateProcess( pi.hProcess, 0 );
				bCanceled = true;
				break;
			}
			nAvail = 0;
		}
		if( bCanceled ){
			break;
		}

		if( dwWait == WAIT_OBJECT_0 && ::WaitForSingleObject( pi.hProcess, 0 ) == WAIT_OBJECT_0 ){
			// プロセス終了後、パイプに残ったデータを読み切る
			for( ;; ){
				DWORD nAvail2 = 0;
				if( !::PeekNamedPipe( hReadPipe, nullptr, 0, nullptr, &nAvail2, nullptr ) || nAvail2 == 0 ){
					break;
				}
				DWORD nRead = 0;
				DWORD nToRead = (std::min)( nAvail2, DWORD(sizeof(work)) );
				if( !::ReadFile( hReadPipe, work, nToRead, &nRead, nullptr ) || nRead == 0 ){
					break;
				}
				splitter.Feed( work, size_t(nRead), handler );
			}
			break;
		}
	}

	::CloseHandle( hReadPipe );
	::CloseHandle( pi.hThread );
	::CloseHandle( pi.hProcess );

	return !bCanceled;
}

} // namespace

namespace CGrepRipgrep {

bool IsAsciiOnly( const wchar_t* pszText ) noexcept
{
	if( pszText == nullptr ){
		return true;
	}
	for( const wchar_t* p = pszText; *p != L'\0'; ++p ){
		if( 0x0080 <= static_cast<unsigned int>(*p) ){
			return false;
		}
	}
	return true;
}

bool CanUseRipgrep(
	const SGrepOption&		sGrepOption,
	bool					bIsHandleGrep,
	const wchar_t*			pszKey,
	const CGrepEnumKeys&	cGrepEnumKeys
) noexcept
{
	if( !sGrepOption.bGrepUseRipgrep ) return false;
	if( sGrepOption.bGrepReplace ) return false;
	if( bIsHandleGrep ) return false;					// 編集中バッファ(:HWND:)の検索はrg非対応
	if( sGrepOption.bGrepStdout ) return false;		// 標準出力モードは既存経路を維持
	if( sGrepOption.nGrepOutputLineType != 1 ) return false;	// 該当部分(0)・否該当行(2)は非対応。該当行(1)のみ対応
	if( pszKey == nullptr || pszKey[0] == L'\0' ) return false;	// ファイル名検索(rgの用途ではない)
	if( !cGrepEnumKeys.m_vecExceptAbsFileKeys.empty() ) return false;		// 絶対パスの除外指定は非対応
	if( !cGrepEnumKeys.m_vecExceptAbsFolderKeys.empty() ) return false;	// 絶対パスの除外指定は非対応

	if( CODE_AUTODETECT == sGrepOption.nGrepCharSet ){
		// SJIS/EUC-JP/UTF-8はASCII範囲のバイト列が共通のため、
		// 検索キーがASCIIのみであれば自動判別のままでも結果は一致する。
		if( !IsAsciiOnly( pszKey ) ) return false;
	}else{
		std::wstring strDummyLabel;
		if( !TryGetRipgrepEncodingLabel( sGrepOption.nGrepCharSet, &strDummyLabel ) ) return false;
	}

	return true;
}

bool TryGetRipgrepEncodingLabel( ECodeType nGrepCharSet, std::wstring* pStrLabel ) noexcept
{
	// rg.exe --encoding が受け付けるWHATWG Encoding仕様のラベル
	// https://encoding.spec.whatwg.org/#concept-encoding-get
	switch( nGrepCharSet ){
	case CODE_SJIS:
		*pStrLabel = L"shift_jis";
		return true;
	case CODE_JIS:
		*pStrLabel = L"iso-2022-jp";
		return true;
	case CODE_EUC:
		*pStrLabel = L"euc-jp";
		return true;
	case CODE_UTF8:
		*pStrLabel = L"utf-8";
		return true;
	case CODE_UNICODE:			// == CODE_UTF16LE
		*pStrLabel = L"utf-16le";
		return true;
	case CODE_UNICODEBE:		// == CODE_UTF16BE
		*pStrLabel = L"utf-16be";
		return true;
	case CODE_LATIN1:
		*pStrLabel = L"windows-1252";
		return true;
	default:
		// CODE_AUTODETECT、CODE_UTF7、CODE_CESU8、コードページ直接指定 等は非対応。
		// 特にCODE_AUTODETECTはrg.exe側では BOM しか判別できず、
		// BOM無しShift-JISファイルなどが無言でヒットしなくなる事故を避けるため
		// 意図的に非対応としている(呼び出し元のCanUseRipgrepで弾かれる)。
		return false;
	}
}

bool TryGetRipgrepVersion( const std::wstring& strExePath, std::wstring* pStrVersion )
{
	if( strExePath.empty() || pStrVersion == nullptr ){
		return false;
	}

	std::wstring strCmdLine;
	AppendQuotedArg( strCmdLine, strExePath );
	strCmdLine += L" --version";

	SECURITY_ATTRIBUTES sa = { sizeof(sa) };
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = nullptr;

	HANDLE hReadPipe = nullptr;
	HANDLE hWritePipe = nullptr;
	if( !::CreatePipe( &hReadPipe, &hWritePipe, &sa, 0 ) ){
		return false;
	}
	::SetHandleInformation( hReadPipe, HANDLE_FLAG_INHERIT, 0 );

	STARTUPINFOW sui = { sizeof(sui) };
	sui.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	sui.wShowWindow = SW_HIDE;
	sui.hStdOutput = hWritePipe;
	sui.hStdError = hWritePipe;

	PROCESS_INFORMATION pi{};
	std::vector<wchar_t> vCmdLineBuf( strCmdLine.begin(), strCmdLine.end() );
	vCmdLineBuf.push_back( L'\0' );

	BOOL bCreated = ::CreateProcessW(
		strExePath.c_str(), vCmdLineBuf.data(), nullptr, nullptr, TRUE,
		CREATE_NO_WINDOW, nullptr, nullptr, &sui, &pi );
	::CloseHandle( hWritePipe );
	if( !bCreated ){
		::CloseHandle( hReadPipe );
		return false;
	}

	constexpr DWORD kTimeoutMillisec = 3000;
	const DWORD dwWaitResult = ::WaitForSingleObject( pi.hProcess, kTimeoutMillisec );

	std::string strOutput;
	if( dwWaitResult == WAIT_OBJECT_0 ){
		char work[512];
		DWORD nAvail = 0;
		while( ::PeekNamedPipe( hReadPipe, nullptr, 0, nullptr, &nAvail, nullptr ) && 0 < nAvail ){
			DWORD nRead = 0;
			DWORD nToRead = (std::min)( nAvail, DWORD(sizeof(work)) );
			if( !::ReadFile( hReadPipe, work, nToRead, &nRead, nullptr ) || nRead == 0 ){
				break;
			}
			strOutput.append( work, nRead );
			nAvail = 0;
		}
	}else{
		::TerminateProcess( pi.hProcess, 0 );
	}

	::CloseHandle( hReadPipe );
	::CloseHandle( pi.hThread );
	::CloseHandle( pi.hProcess );

	if( dwWaitResult != WAIT_OBJECT_0 || strOutput.empty() ){
		return false;
	}

	const size_t nEol = strOutput.find_first_of( "\r\n" );
	const size_t nFirstLineLen = ( nEol == std::string::npos ) ? strOutput.size() : nEol;

	CMemory cmem( strOutput.data(), nFirstLineLen );
	CNativeW cnw;
	CUtf8::UTF8ToUnicode( cmem, &cnw );
	pStrVersion->assign( cnw.GetStringPtr(), cnw.GetStringLength() );
	return !pStrVersion->empty();
}

std::wstring ResolveRipgrepPath( const wchar_t* pszConfiguredPath )
{
	// (1) 設定で明示指定されたパス
	if( pszConfiguredPath && pszConfiguredPath[0] != L'\0' && IsFileExists( pszConfiguredPath, true ) ){
		return pszConfiguredPath;
	}

	// (2) sakura.exeと同じフォルダー(diff.exeと同じ流儀)
	{
		WCHAR szCandidate[_MAX_PATH] = {};
		GetExedir( szCandidate, L"rg.exe" );
		if( IsFileExists( szCandidate, true ) ){
			return szCandidate;
		}
	}

	// (3) PATH環境変数
	{
		WCHAR szCandidate[_MAX_PATH] = {};
		WCHAR* pszFilePart = nullptr;
		DWORD nLen = ::SearchPathW( nullptr, L"rg.exe", nullptr, DWORD(std::size(szCandidate)), szCandidate, &pszFilePart );
		if( 0 < nLen && nLen < std::size(szCandidate) ){
			return szCandidate;
		}
	}

	return std::wstring();
}

std::wstring BuildRipgrepArguments(
	const wchar_t*					pszKey,
	const CGrepEnumKeys&			cGrepEnumKeys,
	const SSearchOption&			sSearchOption,
	const SGrepOption&				sGrepOption,
	const std::vector<std::wstring>& vPaths
)
{
	std::wstring strArgs;
	auto Append = [&strArgs]( const std::wstring& strArg ){ AppendQuotedArg( strArgs, strArg ); };

	Append( L"--no-config" );
	Append( L"--null" );
	Append( L"--column" );
	Append( L"--line-number" );
	Append( L"--no-heading" );
	Append( L"--color=never" );
	// サクラエディタ本体の再帰的なファイル走査は.gitignore等を一切考慮しないため、
	// rg.exe側でも無視ファイル・隠しファイルの暗黙的なスキップを止めて動作を合わせる。
	Append( L"--no-ignore" );
	Append( L"--hidden" );

	if( !sSearchOption.bLoHiCase ){
		Append( L"-i" );
	}
	if( !sSearchOption.bRegularExp ){
		Append( L"-F" );
	}
	if( sSearchOption.bWordOnly ){
		Append( L"-w" );
	}
	if( !sGrepOption.bGrepSubFolder ){
		Append( L"--max-depth=1" );
	}
	if( sGrepOption.bGrepOutputFileOnly ){
		Append( L"--max-count=1" );
	}
	{
		std::wstring strLabel;
		if( TryGetRipgrepEncodingLabel( sGrepOption.nGrepCharSet, &strLabel ) ){
			Append( L"--encoding=" + strLabel );
		}
	}

	for( const LPCWSTR pszPattern : cGrepEnumKeys.m_vecSearchFileKeys ){
		Append( L"--glob=" + std::wstring(pszPattern) );
	}
	for( const LPCWSTR pszPattern : cGrepEnumKeys.m_vecExceptFileKeys ){
		Append( L"--glob=!" + std::wstring(pszPattern) );
	}
	for( const LPCWSTR pszPattern : cGrepEnumKeys.m_vecExceptFolderKeys ){
		Append( L"--glob=!" + std::wstring(pszPattern) );
	}

	Append( L"-e" );
	Append( pszKey );
	Append( L"--" );
	for( const auto& strPath : vPaths ){
		Append( strPath );
	}
	return strArgs;
}

int ConvertUtf8ByteOffsetToUtf16Length( const char* pszUtf8Text, size_t nByteOffset )
{
	if( pszUtf8Text == nullptr || nByteOffset == 0 ){
		return 0;
	}
	CMemory cmem( pszUtf8Text, nByteOffset );
	CNativeW cnw;
	CUtf8::UTF8ToUnicode( cmem, &cnw );
	return int( cnw.GetStringLength() );
}

bool ParseRipgrepHitLine( const char* pszUtf8Line, size_t nLen, SRipgrepHit* pHit )
{
	if( pszUtf8Line == nullptr || pHit == nullptr ){
		return false;
	}

	// パス部分(NULまで)
	size_t nPathLen = 0;
	while( nPathLen < nLen && pszUtf8Line[nPathLen] != '\0' ){
		++nPathLen;
	}
	if( nPathLen == 0 || nPathLen >= nLen ){
		return false;	// NULが見つからない(rgの警告行等)、またはパスが空
	}

	const char* pszRest = pszUtf8Line + nPathLen + 1;
	size_t nRestLen = nLen - nPathLen - 1;
	size_t i = 0;

	// 行番号
	if( i >= nRestLen || !std::isdigit(static_cast<unsigned char>(pszRest[i])) ){
		return false;
	}
	long long nLine = 0;
	while( i < nRestLen && std::isdigit(static_cast<unsigned char>(pszRest[i])) ){
		nLine = nLine * 10 + (pszRest[i] - '0');
		++i;
	}
	if( i >= nRestLen || pszRest[i] != ':' ){
		return false;
	}
	++i;

	// 桁番号(UTF-8バイトオフセット、1始まり)
	if( i >= nRestLen || !std::isdigit(static_cast<unsigned char>(pszRest[i])) ){
		return false;
	}
	long long nByteColumn = 0;
	while( i < nRestLen && std::isdigit(static_cast<unsigned char>(pszRest[i])) ){
		nByteColumn = nByteColumn * 10 + (pszRest[i] - '0');
		++i;
	}
	if( i >= nRestLen || pszRest[i] != ':' ){
		return false;
	}
	++i;

	const char* pszText = pszRest + i;
	size_t nTextLen = nRestLen - i;

	{
		CMemory cmemPath( pszUtf8Line, nPathLen );
		CNativeW cnwPath;
		CUtf8::UTF8ToUnicode( cmemPath, &cnwPath );
		pHit->strFullPath.assign( cnwPath.GetStringPtr(), cnwPath.GetStringLength() );
	}
	{
		CMemory cmemText( pszText, nTextLen );
		CNativeW cnwText;
		CUtf8::UTF8ToUnicode( cmemText, &cnwText );
		pHit->strLineText.assign( cnwText.GetStringPtr(), cnwText.GetStringLength() );
	}

	pHit->nLine = nLine;
	size_t nByteOffset = ( 1 <= nByteColumn ) ? size_t(nByteColumn - 1) : 0;
	if( nByteOffset > nTextLen ){
		nByteOffset = nTextLen;
	}
	pHit->nColumn = 1 + ConvertUtf8ByteOffsetToUtf16Length( pszText, nByteOffset );

	return true;
}

SGrepPathParts ComputeGrepPathParts(
	const wchar_t*	pszFullPath,
	const wchar_t*	pszBaseFolder,
	bool			bGrepSeparateFolder,
	bool			bGrepOutputBaseFolder
)
{
	SGrepPathParts parts;

	WCHAR szFolder[_MAX_PATH + 1] = {};
	WCHAR szFile[_MAX_PATH + 1] = {};
	SplitPath_FolderAndFile( pszFullPath, szFolder, szFile );

	const size_t nBaseLen = wcslen( pszBaseFolder );
	std::wstring strRelFromBase;
	if( wcsncmp( pszFullPath, pszBaseFolder, nBaseLen ) == 0 && pszFullPath[nBaseLen] == L'\\' ){
		strRelFromBase = pszFullPath + nBaseLen + 1;
	}else{
		// 想定外(ベースフォルダー配下でないパス)。フルパスをそのまま使う。
		strRelFromBase = pszFullPath;
	}

	parts.strRelPath = bGrepSeparateFolder ? std::wstring(szFile) : strRelFromBase;

	if( bGrepSeparateFolder && bGrepOutputBaseFolder ){
		const size_t nSlash = strRelFromBase.find_last_of( L'\\' );
		parts.strFolder = ( nSlash == std::wstring::npos ) ? std::wstring() : strRelFromBase.substr( 0, nSlash );
	}else{
		parts.strFolder = szFolder;
	}

	return parts;
}

} // namespace CGrepRipgrep

/*!
	@brief Grep実行(ripgrepを使用、1検索対象パス分)

	@retval 0	正常終了
	@retval -1	キャンセルされた
*/
int CGrepAgent::DoGrepRipgrepPath(
	CEditView*				pcViewDst,
	CDlgCancel*				pcDlgCancel,
	const wchar_t*			pszKey,
	CGrepEnumKeys&			cGrepEnumKeys,
	const WCHAR*			pszPath,
	const SSearchOption&	sSearchOption,
	const SGrepOption&		sGrepOption,
	const std::wstring&	strRipgrepExePath,
	bool&					bOutputBaseFolder,
	int*					pnHitCount,
	CNativeW&				cmemMessage,
	[[maybe_unused]] CNativeW& cUnicodeBuffer	// rgの出力は独自にデコードするため未使用
)
{
	const std::vector<std::wstring> vSinglePath{ pszPath };
	const std::wstring strArgs = CGrepRipgrep::BuildRipgrepArguments( pszKey, cGrepEnumKeys, sSearchOption, sGrepOption, vSinglePath );

	bool bOutputFolderName = false;
	std::wstring strCurrentFullPath;
	std::wstring strCurrentFolder;
	std::wstring strCurrentRelPath;
	BOOL bOutFileName = FALSE;

	auto onHit = [&]( const char* pszLine, size_t nLen )
	{
		CGrepRipgrep::SRipgrepHit hit;
		if( !CGrepRipgrep::ParseRipgrepHitLine( pszLine, nLen, &hit ) ){
			return;	// rgの警告等、想定した書式でない行は無視する
		}

		const DWORD dwNow = ::GetTickCount();
		if( dwNow - m_dwTickUICheck > kUiCheckIntervalMillisec ){
			m_dwTickUICheck = dwNow;
			CEditWnd::getInstance()->SetDrawSwitchOfAllViews(
				0 != ::IsDlgButtonChecked( pcDlgCancel->GetHwnd(), IDC_CHECK_REALTIMEVIEW )
			);
		}
		if( dwNow - m_dwTickUIFileName > kUiFileNameIntervalMillisec ){
			m_dwTickUIFileName = dwNow;
			ApiWrap::DlgItem_SetText( pcDlgCancel->GetHwnd(), IDC_STATIC_CURFILE, hit.strFullPath.c_str() );
		}

		if( strCurrentFullPath != hit.strFullPath ){
			strCurrentFullPath = hit.strFullPath;
			auto parts = CGrepRipgrep::ComputeGrepPathParts(
				hit.strFullPath.c_str(), pszPath, sGrepOption.bGrepSeparateFolder, sGrepOption.bGrepOutputBaseFolder );
			if( strCurrentFolder != parts.strFolder ){
				strCurrentFolder = parts.strFolder;
				bOutputFolderName = false;
			}
			strCurrentRelPath = parts.strRelPath;
			bOutFileName = FALSE;
		}

		OutputPathInfo(
			cmemMessage, sGrepOption,
			strCurrentFullPath.c_str(), pszPath, strCurrentFolder.c_str(), strCurrentRelPath.c_str(),
			L"",	// 文字コード表記: 自動判別時はrgを使わないため常に空
			bOutputBaseFolder, bOutputFolderName, bOutFileName
		);

		const WCHAR* pszDispFilePath = ( sGrepOption.bGrepSeparateFolder || sGrepOption.bGrepOutputBaseFolder )
			? strCurrentRelPath.c_str() : strCurrentFullPath.c_str();

		this->SetGrepResult(
			cmemMessage, pszDispFilePath, L"",
			hit.nLine, hit.nColumn,
			hit.strLineText.c_str(), int(hit.strLineText.length()), 0,
			hit.strLineText.c_str(), int(hit.strLineText.length()),
			sGrepOption
		);
		++(*pnHitCount);
		::SetDlgItemInt( pcDlgCancel->GetHwnd(), IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );

		if( 0 < cmemMessage.GetStringLength() && (::GetTickCount() - m_dwTickAddTail) > kAddTailIntervalMillisec ){
			AddTail( pcViewDst, cmemMessage, sGrepOption.bGrepStdout );
			cmemMessage._SetStringLength(0);
		}
	};

	const bool bOk = RunRipgrepAndParseLines( strRipgrepExePath, strArgs, pszPath, pcDlgCancel, onHit );

	ApiWrap::DlgItem_SetText( pcDlgCancel->GetHwnd(), IDC_STATIC_CURFILE, L" " );

	return bOk ? 0 : -1;
}
