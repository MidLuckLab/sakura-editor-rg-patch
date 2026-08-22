# GitHub 公開準備

このリポジトリを `MidLuckLab` の GitHub へ公開するためのチェックリストです。対象はまず**ソースコードの公開**です。インストーラーや `rg.exe` は配布しません。

## 推奨するリポジトリ設定

| 項目 | 推奨値 |
| --- | --- |
| 所有者 | `MidLuckLab` |
| リポジトリ名 | `sakura-editor-rg-patch` |
| 説明 | `Unofficial Sakura Editor fork with ripgrep-enabled Grep.` |
| 可視性 | Public |
| 既定ブランチ | 公開対象として確認済みのブランチ |
| ライセンス表示 | 既存の `LICENSE`（zlib License）を保持する。GitHub 上で別のライセンスを追加しない。 |

公開先の GitHub リポジトリは空で作成します。作成時に README、`.gitignore`、LICENSE を GitHub 側で追加しないでください。ローカルの履歴とライセンスをそのまま公開するためです。

## 公開前チェックリスト

- [x] ルートの `LICENSE` を保持している。
- [x] README に非公式フォークであること、元プロジェクト、主な変更点を記載している。
- [x] [THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md) に第三者コンポーネントと確認箇所を記載している。
- [x] `rg.exe` を同梱しない方針を明記している。
- [ ] `git status` を確認し、意図しない生成物・秘密情報が含まれていないことを確認する。
- [ ] GitHub のリポジトリ説明、トピック、既定ブランチを設定する。
- [ ] フォーク固有の脆弱性報告先（Security Advisories または連絡先）を設定する。
- [ ] 公開対象ブランチを最終レビューし、コミットする。

## remote と push

現在の `origin` は元プロジェクトを参照するため、公開先には使いません。GitHub で空の `MidLuckLab/sakura-editor-rg-patch` を作成した後、公開用 remote を追加してから、確認済みのブランチだけを push します。

```powershell
git remote add midlucklab https://github.com/MidLuckLab/sakura-editor-rg-patch.git
git push -u midlucklab <公開するブランチ名>
```

`<公開するブランチ名>` は、公開直前に `git branch --show-current` で確認した名前に置き換えます。`origin` へ push しないでください。

## 実行ファイルを後日公開する場合

今回の公開対象はソースのみです。後日 `sakura.exe` を GitHub Releases に添付する場合は、次を満たしてから行います。

- Release ビルドとテストの結果を記録する。
- 配布するファイル名、バージョン、SHA-256 ハッシュ、変更点をリリースノートに記載する。
- 実際にリンク・同梱する第三者コンポーネントを [THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md) に照らして再確認する。
- `bregonig` と Universal Ctags を含む成果物について、適用ライセンスと必要な通知を確認する。
- `rg.exe` を同梱するなら、その正確なバージョンのライセンス文と著作権表示を同梱する。

ライセンスに関する最終的な判断が必要なときは、公開前に専門家へ確認してください。

## 参考資料

- [zlib License（公式）](https://zlib.net/zlib_license.html)
- [GitHub Docs: Licensing a repository](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository)
- [GitHub Docs: Adding a license to a repository](https://docs.github.com/en/communities/setting-up-your-project-for-healthy-contributions/adding-a-license-to-a-repository)
- [Universal Ctags repository](https://github.com/universal-ctags/ctags)
