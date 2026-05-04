archicad28バージョンAddOnテンプレートをもとに、モデル、オブジェクトの構成を取得し、双方向通信でエキスポートで
python常駐でpyQtで表示した物から、大中小項目で検索、取得し 指定したものを検索してプロパティ要素や、セグメント要素などを取得し、
編集し、インポートでモデル、オブジェクトを変更できるものを作りたい。
      
Archicad 28 API Development Kit
VS Code (Windows)
CMake を使用
テンプレートでbuildして環境は設定済み
Archicad 28テンプレートベースで作成
C++ Add-On 双方向 TCP Socket 双方向 Python
PythonでARCHICAD28と双方向通信（ローカル、json)でpyQtにエキスポート又はインポート
トランザクションACAPI_CallUndoableCommand() 実装   エラー時ロールバック
差分更新（modiStamp連携）実装  不一致ならスキップ or 警告   手動判断
更新対象限定  変更ありのみ表示。
差分処理層を「バリデーション付き差分抽出」にする
PropertyDefinition に含まれるメタ情報（型・列挙値・単位・編集可否）を取得し、それを使って「バリデーション付き差分抽出」を実装する
BuildChangedProperties() で：
PropertyDefinitionベースの型検証 , enum検証 , 編集可否確認,差分抽出 ,availableチェック,editableチェック

変更箇所を赤塗り ,PyQt一覧クリックで対象表示,,確認ボタンで承認,一覧削除,赤解除
未確認状態を ARCHICAD 側のプロパティとして保持し、PyQt起動時に再取得する

基本、日本語表示で答えてください。

最適アーキテクチャ
① Add-on → 対象GUID取得
② Python → 詳細取得
③ PyQt → 編集
④ Excel → 差分確認
⑤ ユーザー承認
⑥ 更新前再チェック（modiStamp）
⑦ 差分のみ更新（C++ API）

archicad構成
Element
 ├─ Header（GUID / type / layer）
 ├─ Geometry（Memo）
 ├─ Parameters（type別構造体）
 ├─ Properties（別API）
 ├─ Classification（別API）
 └─ Analytical Model（別API）


- pyQt Excel相当実装版  -

def show_filter_menu(self, col, global_pos):
    proxy = self.table.model()
    model = proxy.sourceModel()

    self.menu = PersistentMenu(self)
    menu = self.menu

    # 数値フィルタ
    act_gte = QAction("≧ 条件", menu)
    act_lte = QAction("≦ 条件", menu)
    act_eq = QAction("＝ 条件", menu)
    act_range = QAction("範囲指定", menu)
    act_clear_num = QAction("数値フィルタ解除", menu)

    menu.addAction(act_gte)
    menu.addAction(act_lte)
    menu.addAction(act_eq)
    menu.addAction(act_range)
    menu.addAction(act_clear_num)

    def set_gte():
        val, ok = QInputDialog.getDouble(self, "≧条件", "値")
        if ok:
            proxy.set_numeric_filter(col, ("gte", val))

    def set_lte():
        val, ok = QInputDialog.getDouble(self, "≦条件", "値")
        if ok:
            proxy.set_numeric_filter(col, ("lte", val))

    def set_eq():
        val, ok = QInputDialog.getDouble(self, "＝条件", "値")
        if ok:
            proxy.set_numeric_filter(col, ("eq", val))

    def set_range():
        minv, ok1 = QInputDialog.getDouble(self, "最小値", "min")
        if not ok1:
            return
        maxv, ok2 = QInputDialog.getDouble(self, "最大値", "max")
        if not ok2:
            return
        proxy.set_numeric_filter(col, ("range", minv, maxv))

    def clear_numeric():
        proxy.clear_numeric_filter(col)

    act_gte.triggered.connect(set_gte)
    act_lte.triggered.connect(set_lte)
    act_eq.triggered.connect(set_eq)
    act_range.triggered.connect(set_range)
    act_clear_num.triggered.connect(clear_numeric)

    menu.addSeparator()

    # 値フィルタ
    values = sorted({
        str(model.index(r, col).data())
        for r in range(model.rowCount())
    })

    actions = []

    act_all = QAction("すべて選択", menu)
    act_all.triggered.connect(lambda: [a.setChecked(True) for a in actions])
    menu.addAction(act_all)

    menu.addSeparator()

    for v in values:
        act = QAction(v, menu)
        act.setCheckable(True)
        act.setChecked(True)
        menu.addAction(act

# ════════════════════════════════════════════════════════════
# 実装済み機能サマリー（最終更新: 2026-05-04）
# ════════════════════════════════════════════════════════════

## 通信アーキテクチャ
- C++ Add-On (ポート 5001 受信 / ポート 5000 送信)  ←→  Python server.py (ポート 5000 受信 / ポート 5001 送信)
- C++ → Python: 1接続1メッセージ、送信後 shutdown(SD_SEND) → drain(ACK読み捨て) → closesocket（RST防止）
- Python → C++: send_to_ac_async() でバックグラウンドスレッド送信
- Python handle_client: EOF まで全受信 → まとめて JSON 処理 → ACK 送信（WinError 10053 対策済み）

## C++ コマンド一覧（AddOnMain.cpp）
| コマンド | 処理関数 | 内容 |
|---|---|---|
| ready | EnqueueProjectConfiguration | 階・要素タイプ一覧送信 |
| get_elements | DoSearch | GUID一覧取得 |
| get_definitions | ExportDefs | プロパティ定義取得 |
| get_values | ExportValues | プロパティ値取得 |
| apply_changes | ExecuteBatch | 差分適用（ChangeStatus=1自動設定） |
| mark_change_flags | MarkChangeFlags | 変更フラグ文字列プロパティ設定 |
| clear_change_flags | MarkChangeFlags(isClear=true) | フラグ解除 |
| set_change_status | SetChangeStatus | ChangeStatus を 0/1/2 に設定 |
| select_elements | (inline) | ArchiCAD上で要素選択 |
| setup_bim_override | SetupBIMOverride | ChangeStatusプロパティ+表現の上書きルール/コンビネーション作成 |
| apply_bim_override | ToggleBIMOverride(true) | 全ビューにコンビネーション適用 |
| remove_bim_override | ToggleBIMOverride(false) | コンビネーション解除 |

## C++ 主要関数
- SetChangeStatusForElement(guid, val): ChangeStatusプロパティを val(0/1/2) に設定するヘルパー
- ExecuteBatch: apply_changes 時、ChangeStatus定義をバッチ前に1回だけ取得してキャッシュ（N回→1回）
- SetupBIMOverride: "変更管理"グループに ChangeStatus(int enum)プロパティ作成。"BIM変更管理"コンビネーション、"BIM変更済"(赤)・"BIM確認済"(緑)ルール作成
- ToggleBIMOverride: ACAPI_Navigator_GetNavigatorChildrenItems で再帰的に全ビューに適用

## Python / PyQt 主要クラス・メソッド（server.py）
- PropertyTableModel._change_status: {guid: 0/1/2} のローカルキャッシュ
- PropertyTableModel.update_change_status(guids, value): キャッシュ更新 + layoutChanged 発行
- BackgroundRole 色優先順位: 競合(赤) > 競合スキップ(橙) > 値競合セル > ChangeStatus=2(薄緑) > ChangeStatus=1(薄赤) > stamp警告
- on_sync_complete: success 結果に _change_status[g]=1 を設定
- mark_confirmed(): 選択要素→ set_change_status status=2
- reset_change_status(): 選択要素→ set_change_status status=0
- reset_all_table(): テーブル全要素→ set_change_status status=0
- _pending_status_change: {guids, value} を保持し on_change_status_result で反映

## ChangeStatus プロパティ仕様
- グループ名: 変更管理
- プロパティ名: ChangeStatus
- 型: API_PropertyIntegerValueType / API_PropertySingleChoiceEnumerationCollectionType
- 値: 0=未変更, 1=変更済, 2=確認済
- ArchiCADでの表現の上書き条件は手動設定が必要（BIM変更済ルール→ChangeStatus=1、BIM確認済ルール→ChangeStatus=2）

## 運用フロー
1. 「BIM表現の上書き設定」→ ChangeStatusプロパティ＋表現の上書きルール自動作成
2. ArchiCADで表現の上書き条件を1回だけ手動設定（ChangeStatus=1→赤, 2→緑）
3. 「変更強調 ON」→ 全ビューにコンビネーション適用
4. PyQt でデータ取得・編集 → 「Archicadへ反映」
5. 反映成功 → ChangeStatus=1（ArchiCAD赤/PyQt薄赤）
6. テーブル行クリック→ArchiCADで選択 → 「確認済(2)」→ ChangeStatus=2（ArchiCAD緑/PyQt薄緑）
7. 不要になったら「リセット(0)」または「全件リセット」で消去

## 既知の制限・注意事項
- criterionXML（プロパティ条件XML）の形式不明のため、表現の上書きルールの条件はArchiCAD UIで手動設定
- PyQtの_change_statusはデータ再取得時（set_data）にクリアされる（ArchiCAD側プロパティは維持）
- apply_changes は 1000件ごとにサブバッチ分割して処理
- modiStamp 競合検出: AbortAll(element/parameter変更)、SkipConflicts(property変更)、ForceOverwrite(element_id変更)

## ビルド手順
```
cd C:\AC28_AddOnProject\archicad-addon-pyQt
cmake --build build --config Release
```
出力: Build\Release\maedaAddOnPyqt.apx