# MidLuckLab contributions and license

MidLuckLab が著作権を保有するこのリポジトリへの独自の変更・追加部分は、[MIT License](./LICENSE-MidLuckLab-MIT.txt) の下で追加許諾します。

## 適用範囲

この追加許諾の対象は、`feature/ripgrep-grep` ブランチにある MidLuckLab 独自の ripgrep Grep 統合、既定設定の変更、テスト、および関連ドキュメントです。主な導入コミットは次のとおりです。

- `19a7eda4e` — ripgrep（`rg.exe`）を利用する Grep 高速パス
- `87e2de58f` — ripgrep Grep の既定有効化
- `f4dfe56d9` — GitHub 公開準備文書

上記の対象には、上流ファイルを変更した箇所と新規に追加したファイルが含まれます。混在するファイルでは、Sakura Editor および第三者の既存部分はそれぞれの元のライセンスに従い、MidLuckLab が著作権を保有する差分のみが MIT License の対象です。

## 上流・第三者ライセンスとの関係

- Sakura Editor の既存ソースおよびリポジトリ全体の基本ライセンスは、ルートの [LICENSE](./LICENSE) にある zlib License のままです。このファイルで置き換えません。
- 第三者コンポーネントには、それぞれのライセンスが適用されます。確認箇所は [THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md) を参照してください。
- MidLuckLab が著作権を保有しないコード、または上流・第三者コードに由来する部分について、MIT License は追加の権利を与えません。

この文書はライセンスの適用範囲を分かりやすくするためのものであり、上流または第三者の著作権表示・ライセンス通知を削除または変更するものではありません。
