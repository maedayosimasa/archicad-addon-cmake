#pragma once

#ifndef GS_EXAMPLE_PRECOMPILED_HEADER_HPP
#define GS_EXAMPLE_PRECOMPILED_HEADER_HPP

// ======================================================
// ① 最重要：Archicad API
// ======================================================
#include "ACAPinc.h"

// ======================================================
// ② 標準ライブラリ
// ======================================================
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// ======================================================
// ③ GSRoot（Archicad基盤）
// ======================================================
#include "GSRoot.hpp"
#include "Array.hpp"
#include "UniString.hpp"

// ======================================================
// ④ DG（UI系）
// ======================================================
#include "DGModule.hpp"
#include "DGDialog.hpp"
#include "DGListBox.hpp"
#include "DGListBox.hpp"
#include "DGCheckItem.hpp"
#include "DGButton.hpp"

// ======================================================
// ⑤ BM（重要：BMMin用）
// ======================================================
#include "BM.hpp"

// ======================================================
// ⑥ Windows専用
// ======================================================
#if defined (WINDOWS)
#include "Win32Interface.hpp"
#endif

#endif // GS_EXAMPLE_PRECOMPILED_HEADER_HPP