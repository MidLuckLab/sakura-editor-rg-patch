# Third-party notices

この文書は、このリポジトリを公開・配布するときに確認する第三者コンポーネントの案内です。各コンポーネントの正式な条件は、リポジトリ内にある原文のライセンスファイルと、実際に配布するバイナリに含まれる成果物で確認してください。この文書自体は法的助言ではありません。

## この変更版について

本リポジトリは Sakura Editor を基にした変更版です。ルートの [LICENSE](./LICENSE) は Sakura Editor Organization による zlib License であり、削除・書き換えをしません。変更したソースはフォークであることと変更内容が分かる形で表示します。

## 確認対象

| コンポーネント | 用途 | 確認するライセンス情報 |
| --- | --- | --- |
| Sakura Editor | 本体 | ルートの [LICENSE](./LICENSE)（zlib License） |
| Onigmo | 正規表現エンジン | [externals/Onigmo/COPYING](./externals/Onigmo/COPYING) |
| Universal Ctags | タグ解析機能 | [externals/ctags/COPYING](./externals/ctags/COPYING) と同梱する通知 |
| cmigemo | Migemo 検索 | `externals/cmigemo/doc/LICENSE_MIT.txt` |
| darkmodelib | ダークモード対応 | `externals/darkmodelib/LICENSE-MIT.md` |
| dll-plugin1 | プラグイン基盤 | `tools/vcpkg-local-registry/ports/dll-plugin1/vcpkg.json` と取得元のライセンス |
| bregonig | 正規表現 DLL | `tools/vcpkg-local-registry/ports/bregonig/vcpkg.json` と、実際に組み込んだパッケージのライセンス |

`bregonig` のポート定義には `GPL-1.0-or-later OR Artistic-1.0-Perl` と記載されています。バイナリを公開する前に、採用したパッケージ・バージョンとライセンス選択を必ず確認してください。

## ripgrep について

このリポジトリおよび今回の `sakura.exe` には `rg.exe` を同梱しません。Grep の高速検索は、利用者が別途用意した `rg.exe` を検出して使用します。将来 `rg.exe` を同梱する場合は、同梱する正確な ripgrep のバージョンに対応するライセンス文・著作権表示を配布物へ追加してください。

## 公開・配布時の手順

- ルートの `LICENSE`、上表のライセンス原文、著作権表示を保持する。
- ソースを変更した箇所と、非公式フォークであることを README とリリースノートで明示する。
- 実行ファイルを配布する場合は、そのファイルに実際にリンク・同梱される第三者コンポーネントを再確認し、必要な通知を配布物に含める。
- ライセンスの選択、商用利用、再配布の可否に判断が必要な場合は、公開前にライセンスに詳しい担当者へ確認する。
