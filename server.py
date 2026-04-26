import sys
import socket
import json
import threading
import time
import os

# PyQt6 のインポート
try:
    from PyQt6.QtWidgets import (QApplication, QMainWindow, QTableView, QVBoxLayout, 
                                 QWidget, QHBoxLayout, QPushButton, QCheckBox, 
                                 QGroupBox, QScrollArea, QLabel)
    from PyQt6.QtCore import Qt, QAbstractTableModel, pyqtSignal, QObject
except ImportError:
    print("PyQt6 not found.")
    sys.exit(1)

# --- テーブルモデル ---
class ElementModel(QAbstractTableModel):
    def __init__(self, data=None):
        super().__init__()
        self._data = data or [] 
        self._headers = ["GUID", "タイプ", "階", "要素ID", "カテゴリID", "構造機能", "位置", "幅", "高さ"]
    def rowCount(self, p=None): return len(self._data)
    def columnCount(self, p=None): return len(self._headers)
    def data(self, index, role=Qt.ItemDataRole.DisplayRole):
        if role == Qt.ItemDataRole.DisplayRole:
            row = self._data[index.row()]
            col = index.column()
            keys = ['guid', 'type', 'floor', 'elementID', 'categoryID', 'structuralFunction', 'position', 'width', 'height']
            val = row.get(keys[col], "")
            if keys[col] in ['width', 'height']:
                try: return f"{float(val):.2f}"
                except: return str(val)
            return str(val)
        return None
    def headerData(self, section, orientation, role=Qt.ItemDataRole.DisplayRole):
        if role == Qt.ItemDataRole.DisplayRole and orientation == Qt.Orientation.Horizontal:
            return self._headers[section]
        return None
    def update_data(self, new_data):
        self.beginResetModel()
        self._data = new_data
        self.endResetModel()

# --- 通信信号 ---
class CommunicationSignals(QObject):
    data_received = pyqtSignal(dict)
    config_received = pyqtSignal(dict)

# --- TCP サーバー ---
class TcpServerThread(threading.Thread):
    def __init__(self, signals):
        super().__init__(daemon=True)
        self.signals = signals
    def run(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                s.bind(('127.0.0.1', 5000))
                s.listen()
                while True:
                    conn, addr = s.accept()
                    with conn:
                        data = b""
                        while True:
                            chunk = conn.recv(65536)
                            if not chunk: break
                            data += chunk
                        if data:
                            payload = json.loads(data.decode('utf-8'))
                            if payload.get("type") == "project_config": self.signals.config_received.emit(payload)
                            else: self.signals.data_received.emit(payload)
            except Exception: pass

# --- メインウィンドウ ---
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Archicad - PyQt 連携パネル")
        self.resize(1200, 750)
        self.setStyleSheet("""
            QPushButton#SearchBtn { background-color: #2196F3; color: white; border-radius: 4px; font-weight: bold; font-size: 14px; min-height: 40px; }
            QPushButton#SearchBtn:hover { background-color: #1976D2; }
            QPushButton#CloseBtn { background-color: #f44336; color: white; border-radius: 4px; font-weight: bold; min-height: 40px; }
            QPushButton#CloseBtn:hover { background-color: #d32f2f; }
            QGroupBox { font-weight: bold; border: 1px solid #ccc; margin-top: 10px; padding-top: 15px; }
        """)

        central = QWidget(); self.setCentralWidget(central)
        layout = QHBoxLayout(central)

        left_panel = QWidget(); left_panel.setFixedWidth(280)
        left_vbox = QVBoxLayout(left_panel)
        self.story_layout = QVBoxLayout()
        self.type_layout = QVBoxLayout()
        
        sg = QGroupBox("階（フロア）選択"); sg.setLayout(self.story_layout)
        tg = QGroupBox("要素タイプ選択"); tg.setLayout(self.type_layout)
        
        scroll = QScrollArea(); scroll.setWidgetResizable(True)
        container = QWidget(); cv = QVBoxLayout(container)
        cv.addWidget(sg); cv.addWidget(tg); cv.addStretch()
        scroll.setWidget(container)
        left_vbox.addWidget(scroll)
        layout.addWidget(left_panel)

        right_panel = QVBoxLayout()
        self.table = QTableView()
        self.model = ElementModel()
        self.table.setModel(self.model)
        self.status_label = QLabel("待機中")
        
        btn_layout = QHBoxLayout()
        self.btn_search = QPushButton("選択された条件で検索・取得")
        self.btn_search.setObjectName("SearchBtn")
        self.btn_search.clicked.connect(self.request_search)
        
        self.btn_close = QPushButton("閉じる")
        self.btn_close.setObjectName("CloseBtn")
        self.btn_close.setFixedWidth(100)
        self.btn_close.clicked.connect(self.hide)
        
        btn_layout.addWidget(self.btn_search, 1); btn_layout.addWidget(self.btn_close)
        right_panel.addWidget(self.table); right_panel.addWidget(self.status_label); right_panel.addLayout(btn_layout)
        layout.addLayout(right_panel, 1)

        self.signals = CommunicationSignals()
        self.signals.data_received.connect(self.on_data_received)
        self.signals.config_received.connect(self.on_config_received)
        TcpServerThread(self.signals).start()
        self.story_cbs = []; self.type_cbs = {}

    def on_config_received(self, data):
        for l in [self.story_layout, self.type_layout]:
            while l.count():
                w = l.takeAt(0).widget()
                if w: w.setParent(None)
        self.story_cbs = []
        for s in data.get("stories", []):
            cb = QCheckBox(s['name']); cb.setProperty("index", s['index'])
            self.story_layout.addWidget(cb); self.story_cbs.append(cb)
        self.type_cbs = {}
        for t, count in data.get("elementTypes", {}).items():
            cb = QCheckBox(f"{t} ({count})")
            self.type_layout.addWidget(cb); self.type_cbs[t] = cb
            
        self.status_label.setText("構成情報を取得しました。")
        self.show_and_activate()

    def request_search(self):
        stories = [cb.property("index") for cb in self.story_cbs if cb.isChecked()]
        types = {t: True for t, cb in self.type_cbs.items() if cb.isChecked()}
        if not types:
            self.status_label.setText("エラー: 要素タイプを選択してください。")
            return

        self.status_label.setText("検索中...")
        req = {"command": "get_elements", "stories": stories}; req.update(types)
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(2); s.connect(('127.0.0.1', 5001))
                s.sendall(json.dumps(req).encode('utf-8'))
        except Exception as e:
            self.status_label.setText(f"通信エラー: {e}")

    def on_data_received(self, data):
        elements = data.get("elements", [])
        self.model.update_data(elements)
        self.status_label.setText(f"完了: {len(elements)} 件を表示中")
        self.show_and_activate()

    def show_and_activate(self):
        # 最小化を強制解除
        self.setWindowState(self.windowState() & ~Qt.WindowState.WindowMinimized | Qt.WindowState.WindowActive)
        self.show()
        self.showNormal()
        self.raise_()
        self.activateWindow()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setQuitOnLastWindowClosed(False)
    win = MainWindow()
    sys.exit(app.exec())
