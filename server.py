import sys
import socket
import json
import threading
import time
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QTableView, QVBoxLayout,
    QWidget, QHBoxLayout, QPushButton, QGroupBox, QLabel, 
    QTreeWidget, QTreeWidgetItem, QSplitter, QMessageBox, QHeaderView,
    QTreeWidgetItemIterator, QMenu, QInputDialog
)
from PyQt6.QtCore import Qt, QAbstractTableModel, pyqtSignal, QObject, QSortFilterProxyModel
from PyQt6.QtGui import QColor, QAction
from pathlib import Path

LOG_PATH = Path(__file__).resolve().parent / "server_log.txt"


def append_log(message):
    try:
        with LOG_PATH.open("a", encoding="utf-8") as fp:
            fp.write(f"[PY] {message}\n")
    except Exception:
        pass


def normalize_label(value):
    return str(value or "").strip().lower()


def is_element_id_property(prop_def, prop_guid=""):
    name = normalize_label(prop_def.get("name", ""))
    group = normalize_label(prop_def.get("group", ""))
    guid = normalize_label(prop_guid)

    if guid in {"builtin:id", "builtin:elementid", "builtin:element_id"}:
        return True

    id_like = (
        name == "id" or
        name == "element id" or
        name == "要素id" or
        "element id" in name or
        "要素id" in name
    )
    group_like = (
        group == "id and categories" or
        group == "idとカテゴリ" or
        ("categor" in group and "id" in group) or
        ("カテゴリ" in group and "id" in group)
    )

    return id_like and group_like

# ============================================================
# フィルタリングプロキシモデル
# ============================================================
class ExcelFilterProxyModel(QSortFilterProxyModel):
    def __init__(self):
        super().__init__()
        self._filters = {} # col_index -> set of allowed values
        self._numeric_filters = {} # col_index -> (type, val1, val2)

    def set_value_filter(self, col, allowed_values):
        self._filters[col] = set(allowed_values)
        self.invalidateFilter()

    def set_numeric_filter(self, col, filter_data):
        self._numeric_filters[col] = filter_data
        self.invalidateFilter()

    def clear_numeric_filter(self, col):
        if col in self._numeric_filters:
            del self._numeric_filters[col]
            self.invalidateFilter()

    def filterAcceptsRow(self, source_row, source_parent):
        for col, allowed in self._filters.items():
            idx = self.sourceModel().index(source_row, col, source_parent)
            val = str(self.sourceModel().data(idx))
            if val not in allowed: return False

        for col, data in self._numeric_filters.items():
            idx = self.sourceModel().index(source_row, col, source_parent)
            try:
                val_str = str(self.sourceModel().data(idx))
                val = float(val_str)
                ftype = data[0]
                if ftype == "gte" and not (val >= data[1]): return False
                if ftype == "lte" and not (val <= data[1]): return False
                if ftype == "eq" and not (val == data[1]): return False
                if ftype == "range" and not (data[1] <= val <= data[2]): return False
            except: 
                return False 
        return True

# ============================================================
# データモデル
# ============================================================
class PropertyTableModel(QAbstractTableModel):
    def __init__(self):
        super().__init__()
        self._elements = []
        self._properties = []
        self._values = {}
        self._original_values = {}
        self._stamps = {} 

    def set_data(self, elements, properties, values, stamps=None):
        self.beginResetModel()
        self._elements = elements
        self._properties = properties
        self._values = values.copy()
        self._original_values = values.copy()
        if stamps:
            self._stamps.update(stamps)
        self.endResetModel()

    def rowCount(self, parent=None): return len(self._elements)
    def columnCount(self, parent=None): return len(self._properties) + 1

    def data(self, index, role=Qt.ItemDataRole.DisplayRole):
        if not index.isValid() or index.row() >= len(self._elements): return None
        row, col = index.row(), index.column()
        guid = self._elements[row]["guid"]
        if role in (Qt.ItemDataRole.DisplayRole, Qt.ItemDataRole.EditRole):
            if col == 0: return guid[:8]
            prop_guid = self._properties[col - 1]["guid"]
            return str(self._values.get((guid, prop_guid), ""))
        
        if role == Qt.ItemDataRole.BackgroundRole and col > 0:
            prop_guid = self._properties[col - 1]["guid"]
            if str(self._values.get((guid, prop_guid))) != str(self._original_values.get((guid, prop_guid))):
                return QColor("#FFEEEE") 
        return None

    def setData(self, index, value, role=Qt.ItemDataRole.EditRole):
        if index.isValid() and role == Qt.ItemDataRole.EditRole:
            row, col = index.row(), index.column()
            if col == 0: return False
            self._values[(self._elements[row]["guid"], self._properties[col-1]["guid"])] = value
            self.dataChanged.emit(index, index)
            return True
        return False

    def flags(self, index):
        if index.column() == 0: return Qt.ItemFlag.ItemIsEnabled
        return Qt.ItemFlag.ItemIsEnabled | Qt.ItemFlag.ItemIsSelectable | Qt.ItemFlag.ItemIsEditable

    def headerData(self, section, orientation, role=Qt.ItemDataRole.DisplayRole):
        if role == Qt.ItemDataRole.DisplayRole and orientation == Qt.Orientation.Horizontal:
            return "ID" if section == 0 else self._properties[section - 1]["name"]
        return None

# ============================================================
# 通信シグナルとスレッド
# ============================================================
class CommunicationSignals(QObject):
    config_received = pyqtSignal(dict)
    elements_received = pyqtSignal(dict)
    definitions_received = pyqtSignal(dict)
    values_received = pyqtSignal(dict)
    sync_complete = pyqtSignal(dict)
    show_window = pyqtSignal()
    error_occurred = pyqtSignal(str)

class TcpServerThread(threading.Thread):
    def __init__(self, signals):
        super().__init__(daemon=True)
        self.signals = signals
        self._stop_event = threading.Event()

    def stop(self):
        self._stop_event.set()

    def run(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.settimeout(1.0)
            try:
                s.bind(("127.0.0.1", 5000))
                s.listen()
            except Exception as e:
                self.signals.error_occurred.emit(f"Server Bind Error: {e}")
                return

            for _ in range(10):
                if self._stop_event.is_set():
                    break
                try:
                    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as notify_socket:
                        notify_socket.settimeout(0.5)
                        notify_socket.connect(("127.0.0.1", 5001))
                        notify_socket.sendall(json.dumps({"command": "ready"}).encode("utf-8"))
                    break
                except:
                    time.sleep(0.3)

            while not self._stop_event.is_set():
                try:
                    conn, _ = s.accept()
                    with conn:
                        chunks = []
                        while not self._stop_event.is_set():
                            conn.settimeout(1.0)
                            try:
                                chunk = conn.recv(65536)
                                if not chunk: break
                                chunks.append(chunk)
                            except socket.timeout: continue
                        if chunks:
                            try:
                                full_data = b"".join(chunks).decode("utf-8")
                                payload = json.loads(full_data)
                                t = payload.get("type")
                                if t == "project_config": self.signals.config_received.emit(payload); self.signals.show_window.emit()
                                elif t == "elements": self.signals.elements_received.emit(payload)
                                elif t == "property_definitions": self.signals.definitions_received.emit(payload)
                                elif t == "property_values": self.signals.values_received.emit(payload)
                                elif t == "sync_complete": self.signals.sync_complete.emit(payload)
                            except Exception as e: print(f"Parse Error: {e}")
                except socket.timeout: continue
                except Exception as e:
                    if not self._stop_event.is_set(): print(f"Accept Error: {e}")

# ============================================================
# メインウィンドウ
# ============================================================
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Archicad BIM Property Manager")
        self.resize(1100, 700)
        self.setup_ui()
        self.signals = CommunicationSignals()
        self.signals.config_received.connect(self.on_config)
        self.signals.elements_received.connect(self.on_elements)
        self.signals.definitions_received.connect(self.on_definitions)
        self.signals.values_received.connect(self.on_values)
        self.signals.sync_complete.connect(self.on_sync_complete)
        self.signals.show_window.connect(self.show)
        self.signals.error_occurred.connect(lambda m: QMessageBox.warning(self, "エラー", m))
        self.server_thread = None

    def setup_ui(self):
        central = QWidget(); self.setCentralWidget(central)
        layout = QVBoxLayout(central); splitter = QSplitter(Qt.Orientation.Horizontal)
        
        left = QWidget(); l_lay = QVBoxLayout(left)
        self.filter_tree = QTreeWidget(); self.filter_tree.setHeaderLabel("階 / タイプ")
        self.btn_search = QPushButton("要素を抽出"); self.btn_search.clicked.connect(self.search_elements)
        l_lay.addWidget(self.filter_tree); l_lay.addWidget(self.btn_search)
        
        mid = QWidget(); m_lay = QVBoxLayout(mid)
        self.prop_tree = QTreeWidget(); self.prop_tree.setHeaderLabel("プロパティ")
        self.prop_tree.itemChanged.connect(self.on_prop_item_changed)
        self.btn_get_defs = QPushButton("プロパティ一覧を取得"); self.btn_get_defs.clicked.connect(self.request_definitions)
        self.btn_get_values = QPushButton("データ取得"); self.btn_get_values.clicked.connect(self.get_values)
        m_lay.addWidget(self.prop_tree); m_lay.addWidget(self.btn_get_defs); m_lay.addWidget(self.btn_get_values)
        
        right = QWidget(); r_lay = QVBoxLayout(right)
        self.table = QTableView()
        self.model = PropertyTableModel()
        self.proxy = ExcelFilterProxyModel()
        self.proxy.setSourceModel(self.model)
        self.table.setModel(self.proxy)
        self.table.horizontalHeader().setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.table.horizontalHeader().customContextMenuRequested.connect(self.show_filter_menu)
        
        self.btn_sync = QPushButton("Archicadへ反映"); self.btn_sync.clicked.connect(self.sync_data)
        self.status_label = QLabel("待機中"); r_lay.addWidget(self.status_label)
        r_lay.addWidget(self.table); r_lay.addWidget(self.btn_sync)

        self.table.clicked.connect(self.on_table_clicked)

        splitter.addWidget(left); splitter.addWidget(mid); splitter.addWidget(right)
        layout.addWidget(splitter)

    def on_table_clicked(self, index):
        source_index = self.proxy.mapToSource(index)
        row = source_index.row()
        if row < len(self.model._elements):
            guid = self.model._elements[row]["guid"]
            self.send_to_ac_async({"command": "select_elements", "guids": [guid]})

    def show_filter_menu(self, pos):
        col = self.table.horizontalHeader().logicalIndexAt(pos)
        if col < 0: return
        menu = QMenu(self)
        num_menu = menu.addMenu("数値フィルタ")
        act_gte = QAction("≧ 条件", self)
        act_lte = QAction("≦ 条件", self)
        act_eq = QAction("＝ 条件", self)
        act_range = QAction("範囲指定", self)
        act_clear_num = QAction("数値フィルタ解除", self)
        num_menu.addActions([act_gte, act_lte, act_eq, act_range])
        num_menu.addSeparator()
        num_menu.addAction(act_clear_num)
        
        def set_gte():
            val, ok = QInputDialog.getDouble(self, "≧条件", "値")
            if ok: self.proxy.set_numeric_filter(col, ("gte", val))
        def set_lte():
            val, ok = QInputDialog.getDouble(self, "≦条件", "値")
            if ok: self.proxy.set_numeric_filter(col, ("lte", val))
        def set_eq():
            val, ok = QInputDialog.getDouble(self, "＝条件", "値")
            if ok: self.proxy.set_numeric_filter(col, ("eq", val))
        def set_range():
            minv, ok1 = QInputDialog.getDouble(self, "最小値", "min")
            if not ok1: return
            maxv, ok2 = QInputDialog.getDouble(self, "最大値", "max")
            if ok2: self.proxy.set_numeric_filter(col, ("range", minv, maxv))
            
        act_gte.triggered.connect(set_gte)
        act_lte.triggered.connect(set_lte)
        act_eq.triggered.connect(set_eq)
        act_range.triggered.connect(set_range)
        act_clear_num.triggered.connect(lambda: self.proxy.clear_numeric_filter(col))
        
        menu.addSeparator()
        values = sorted({str(self.model.index(r, col).data()) for r in range(self.model.rowCount())})
        actions = []
        act_all = QAction("すべて選択", menu)
        act_all.triggered.connect(lambda: [a.setChecked(True) for _, a in actions])
        menu.addAction(act_all)
        menu.addSeparator()
        for v in values:
            act = QAction(v, menu)
            act.setCheckable(True)
            current_allowed = self.proxy._filters.get(col)
            act.setChecked(current_allowed is None or v in current_allowed)
            menu.addAction(act)
            actions.append((v, act))
        def apply_value_filter():
            allowed = [v for v, a in actions if a.isChecked()]
            self.proxy.set_value_filter(col, allowed)
        menu.aboutToHide.connect(apply_value_filter)
        menu.exec(self.table.horizontalHeader().mapToGlobal(pos))

    def on_prop_item_changed(self, item, column):
        self.prop_tree.blockSignals(True)
        state = item.checkState(0)
        for i in range(item.childCount()):
            item.child(i).setCheckState(0, state)
        self.prop_tree.blockSignals(False)

    def closeEvent(self, event):
        if self.server_thread: self.server_thread.stop()
        event.accept()

    def on_config(self, data):
        self.filter_tree.clear()
        s_root = QTreeWidgetItem(self.filter_tree, ["階層"])
        for s in data.get("stories", []):
            child = QTreeWidgetItem(s_root, [s["name"]])
            child.setData(0, Qt.ItemDataRole.UserRole, s["index"])
            child.setCheckState(0, Qt.CheckState.Unchecked)
        t_root = QTreeWidgetItem(self.filter_tree, ["要素タイプ"])
        for t_name, count in data.get("elementTypes", {}).items():
            child = QTreeWidgetItem(t_root, [f"{t_name} ({count})"])
            child.setData(0, Qt.ItemDataRole.UserRole, t_name)
            child.setCheckState(0, Qt.CheckState.Unchecked)
        self.filter_tree.expandAll()

    def search_elements(self):
        stories, types = [], {}
        it = QTreeWidgetItemIterator(self.filter_tree)
        while it.value():
            item = it.value()
            if item.parent():
                if item.parent().text(0) == "階層":
                    if item.checkState(0) == Qt.CheckState.Checked: stories.append(item.data(0, Qt.ItemDataRole.UserRole))
                else:
                    types[item.data(0, Qt.ItemDataRole.UserRole)] = (item.checkState(0) == Qt.CheckState.Checked)
            it += 1
        self.status_label.setText("要素抽出中...")
        self.send_to_ac_async({"command": "get_elements", "stories": stories, **types})

    def on_elements(self, data):
        self.current_elements = data.get("elements", [])
        count = len(self.current_elements)
        if count > 0: self.status_label.setText(f"要素: {count}件抽出。")
        else: self.status_label.setText("該当なし。")

    def request_definitions(self):
        if not hasattr(self, 'current_elements') or not self.current_elements: return
        self.send_to_ac_async({"command": "get_definitions", "guids": [e["guid"] for e in self.current_elements[:1]]})

    def on_definitions(self, data):
        self.prop_tree.clear()
        groups = {}
        for d in data.get("definitions", []):
            g_name = d.get("group", "Other")
            if g_name not in groups:
                parent = QTreeWidgetItem(self.prop_tree, [g_name])
                parent.setCheckState(0, Qt.CheckState.Unchecked)
                groups[g_name] = parent
            child = QTreeWidgetItem(groups[g_name], [d["name"]])
            child.setData(0, Qt.ItemDataRole.UserRole, d)
            child.setCheckState(0, Qt.CheckState.Unchecked)
        self.prop_tree.expandAll()

    def get_values(self):
        selected_props = []
        it = QTreeWidgetItemIterator(self.prop_tree)
        while it.value():
            item = it.value()
            if item.parent() and item.checkState(0) == Qt.CheckState.Checked:
                data = item.data(0, Qt.ItemDataRole.UserRole)
                if data: selected_props.append(data)
            it += 1
        if not selected_props or not hasattr(self, 'current_elements'): return
        self.current_props = selected_props
        self.send_to_ac_async({
            "command": "get_values", 
            "guids": [e["guid"] for e in self.current_elements], 
            "propGuids": [p["guid"] for p in selected_props]
        })

    def on_values(self, data):
        vals, stamps = {}, {}
        for item in data.get("values", []):
            guid = item["guid"]
            stamps[guid] = item.get("modiStamp", 0)
            for p_guid, v in item["props"].items():
                vals[(guid, p_guid)] = v
        self.model.set_data(self.current_elements, self.current_props, vals, stamps)

    def sync_data(self):
        changes = []
        props = getattr(self, 'current_props', [])
        prop_map = {p['guid']: p for p in props}
        for (guid, p_guid), val in self.model._values.items():
            if str(val) != str(self.model._original_values.get((guid, p_guid))):
                p_def = prop_map.get(p_guid, {})
                special_type = "element_id" if is_element_id_property(p_def, p_guid) else ""
                g_name = p_def.get("group", "")
                if g_name in ["IDとカテゴリ", "ID and Categories", "分類", "Classification"]: group_type = "classification"
                elif g_name in ["レイヤ", "Layer"]: group_type = "element"
                elif g_name in ["一般パラメータ", "General Parameters", "形状", "Geometry", "位置"]: group_type = "parameter"
                elif g_name in ["面と材質", "Surfaces", "材質", "Material", "ビルディングマテリアル"]: group_type = "attribute"
                else: group_type = "property"
                if p_guid.startswith("builtin:Layer"): group_type = "element"
                elif p_guid.startswith("builtin:"): group_type = "parameter"
                prop_name = p_def.get("name", "")
                prop_group = p_def.get("group", "")
                changes.append({
                    "guid": guid, "group": group_type, "propId": p_guid, "propName": prop_name, "propGroup": prop_group, "specialType": special_type, "value": str(val),
                    "modiStamp": str(self.model._stamps.get(guid, 0))
                })
        if changes:
            self.status_label.setText("反映中...")
            first = changes[0]
            append_log(
                f"apply_changes count={len(changes)} first_guid={first.get('guid')} "
                f"propId={first.get('propId')} propName={first.get('propName')} "
                f"propGroup={first.get('propGroup')} specialType={first.get('specialType')} value={first.get('value')}"
            )
            self.send_to_ac_async({"command": "apply_changes", "changes": changes})

    def on_sync_complete(self, data):
        if data.get("status") == "success":
            self.status_label.setText("反映完了")
            self.model._original_values = self.model._values.copy()
            self.model.layoutChanged.emit()
            QMessageBox.information(self, "完了", "反映成功。")
        else:
            QMessageBox.critical(self, "エラー", "反映失敗 (競合発生の可能性があります)。")

    def send_to_ac_async(self, data):
        def _send():
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.settimeout(5.0); s.connect(("127.0.0.1", 5001))
                    s.sendall(json.dumps(data, separators=(',', ':')).encode("utf-8"))
            except Exception as e: self.signals.error_occurred.emit(str(e))
        threading.Thread(target=_send, daemon=True).start()

if __name__ == "__main__":
    app = QApplication(sys.argv); win = MainWindow()
    server_thread = TcpServerThread(win.signals); win.server_thread = server_thread
    server_thread.start(); sys.exit(app.exec())
