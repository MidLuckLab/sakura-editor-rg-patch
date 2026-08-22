# Sakura Editor

A free text editor for Windows.  
[![License: Zlib](https://img.shields.io/badge/License-Zlib-lightgrey.svg)](https://opensource.org/licenses/Zlib)
[![build sakura](https://github.com/sakura-editor/sakura/workflows/build%20sakura/badge.svg)](https://github.com/sakura-editor/sakura/actions?query=workflow%3A%22build+sakura%22)
[![CodeFactor](https://www.codefactor.io/repository/github/sakura-editor/sakura/badge)](https://www.codefactor.io/repository/github/sakura-editor/sakura)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=sakura-editor_sakura&metric=alert_status)](https://sonarcloud.io/dashboard?id=sakura-editor_sakura)
[![Github Releases All](https://img.shields.io/github/downloads/sakura-editor/sakura/total.svg)](https://github.com/sakura-editor/sakura/releases "All Releases")
[![Star History](https://img.shields.io/badge/star-histroy-yellow.svg)](https://star-history.t9t.io/#sakura-editor/sakura)

> **非公式フォーク / 変更版**
>
> このリポジトリは [Sakura Editor](https://github.com/sakura-editor/sakura) を基に、ripgrep を利用する Grep の改善と関連設定を加えた変更版です。公式プロジェクトではありません。元プロジェクトのライセンスは [LICENSE](./LICENSE) を保持し、第三者コンポーネントは [THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md) に整理しています。GitHub 公開時の確認事項は [PUBLISHING.md](./PUBLISHING.md) を参照してください。

<!-- 以下は Markdownの参照形式によるリンク の定義です。 -->
<!-- 参照 https://hail2u.net/blog/coding/markdown-reference-style-links.html -->

[Visual Studio 以前のバージョン]: https://visualstudio.microsoft.com/ja/vs/older-downloads/ "Visual Studio 以前のバージョン"
[Visual Studio 最新版]: https://visualstudio.microsoft.com/ja/downloads/ "Visual Studio 最新版"
[ライセンスの OSI のページ]: https://opensource.org/license/zlib
[Visual Studio Community ライセンス]: https://visualstudio.microsoft.com/ja/license-terms/vs2022-ga-community/
[Markdown をローカルで確認する方法]: https://github.com/sakura-editor/sakura/wiki/markdown-%E3%82%92%E3%83%AD%E3%83%BC%E3%82%AB%E3%83%AB%E3%81%A7%E7%A2%BA%E8%AA%8D%E3%81%99%E3%82%8B%E6%96%B9%E6%B3%95
[How to extract currently installed Visual Studio component IDs?]: https://stackoverflow.com/questions/52946333/how-to-extract-currently-installed-visual-studio-component-ids
[Configure Visual Studio across your organization with .vsconfig]: https://devblogs.microsoft.com/setup/configure-visual-studio-across-your-organization-with-vsconfig/
[インストール構成をインポートまたはエクスポートする]: https://docs.microsoft.com/ja-jp/visualstudio/install/import-export-installation-configurations?view=vs-2019
[コマンド ライン パラメーターを使用して Visual Studio をインストールする]: https://docs.microsoft.com/ja-jp/visualstudio/install/use-command-line-parameters-to-install-visual-studio?view=vs-2019
[不足しているコンポーネントを自動的にインストールする]: https://docs.microsoft.com/ja-jp/visualstudio/install/import-export-installation-configurations?view=vs-2019#automatically-install-missing-components

<!-- TOC -->

- [Sakura Editor](#sakura-editor)
  - [脆弱性の報告方法](#脆弱性の報告方法)
  - [ダウンロード](#ダウンロード)
  - [フォークとライセンス](#フォークとライセンス)
  - [開発情報](#開発情報)
    - [How to build](#how-to-build)
    - [ripgrep を使った Grep](#ripgrep-を使った-grep)
    - [Static code analysis](#static-code-analysis)
  - [変更履歴](#変更履歴)

<!-- /TOC -->

## 脆弱性の報告方法

https://github.com/sakura-editor/sakura/security/advisories
から報告を行ってください。


## ダウンロード

リリース版は [Sakura Editor Portal](https://sakura-editor.github.io/) に置いてあります。

GitHubで公開しているので [GitHub Actionsのビルドページ](https://github.com/sakura-editor/sakura/actions/workflows/build-sakura.yml?query=branch%3Amaster) より開発中の最新版をダウンロードすることもできます。

## フォークとライセンス

このリポジトリは Sakura Editor の非公式な変更版です。変更内容はコミット履歴と本 README に記載し、元の [zlib License](./LICENSE) と著作権表示を保持しています。第三者コンポーネントのライセンスファイルは削除・改変せずに同梱し、一覧と配布時の確認事項を [THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md) に記載しています。

## 開発情報

### How to build

`sakura.sln`を開いてビルドできます。  
詳細は [ビルド方法](./tools/build.md) を参照。

### ripgrep を使った Grep

Grep では、`rg.exe`（ripgrep）を利用する高速検索を既定で有効にしています。`rg.exe` はサクラエディタの実行ファイルと同じフォルダー、環境変数 `PATH`、または設定画面で指定したパスから検出します。見つからない場合や対応外の検索条件では、従来の Grep に自動的に切り替わります。

高速検索を利用するには、Grep ダイアログの「文字コードセット」で **UTF-8** または **Shift_JIS** などの文字コードを明示指定してください。「自動判別」では ripgrep を使用しません。ripgrep の自動判別は BOM の検出に限られ、サクラエディタの自動判別と結果が異なる可能性があるためです。

既存の設定ファイルに `bGrepUseRipgrep` が保存されている場合は、その設定を維持します。

### Static code analysis

サクラエディタプロジェクトではいくつかの静的解析サービスを利用しています。

| サービス | 説明 |
|---------|------|
| [CodeFactor](https://www.codefactor.io/repository/github/sakura-editor/sakura) |  C++ のスタイルチェック。[cpplint.py](./tools/CodeFactor/README.md)による字句解析。 |
| [SonarQube](https://sonarcloud.io/summary/new_code?id=sakura-editor_sakura&branch=master) | 多言語対応の静的解析。Build Wrapper + SonarScannerによるコード品質解析。 |

[![Quality gate](https://sonarcloud.io/api/project_badges/quality_gate?project=sakura-editor_sakura)](https://sonarcloud.io/summary/overall?id=sakura-editor_sakura)

## 変更履歴

- 変更履歴は [CHANGELOG.md](https://github.com/sakura-editor/sakura/blob/master/CHANGELOG.md) を参照してください。
- 最新の `CHANGELOG.md` は [ここからダウンロード](https://ci.appveyor.com/project/sakuraeditor/changelog-sakura/branch/master/artifacts) できます([Markdown をローカルで確認する方法](https://github.com/sakura-editor/sakura/wiki/markdown-%E3%82%92%E3%83%AD%E3%83%BC%E3%82%AB%E3%83%AB%E3%81%A7%E7%A2%BA%E8%AA%8D%E3%81%99%E3%82%8B%E6%96%B9%E6%B3%95)
で説明している手順でローカルで確認できます)。
- `CHANGELOG.md` は [PullRequest](https://github.com/sakura-editor/sakura/pulls) から自動的に生成しています。
  - 具体的には [github-changelog-generator](https://github.com/github-changelog-generator/github-changelog-generator) というソフトを使用して [changelog-sakura](https://github.com/sakura-editor/changelog-sakura) のリポジトリで [appveyor](https://ci.appveyor.com/project/sakuraeditor/changelog-sakura) で生成しています。
  - 詳細は wiki の [CHANGELOG.mdについて](https://github.com/sakura-editor/sakura/wiki/CHANGELOG.md%E3%81%AB%E3%81%A4%E3%81%84%E3%81%A6) をご覧ください。
