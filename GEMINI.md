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