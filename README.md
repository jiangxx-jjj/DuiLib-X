# DuiLib-X

[English](README.md) | [中文](README.zh-CN.md)

**DuiLib-X** 是 Windows 平台 DirectUI 界面库 DuiLib 的增强分支。在原库基础上增加了 **HTML/CSS 界面渲染能力**：前端用 HTML/CSS 写界面，后端用 C++ 写逻辑，完全不依赖浏览器内核。

## 核心特性

### HTML/CSS 渲染引擎（litehtml 驱动）

- **排版引擎**：[litehtml](https://github.com/litehtml/litehtml)（BSD-3 许可，C++，非浏览器内核）
- **绘制层**：GDI+（抗锯齿、圆角路径、渐变、真半透明）
- **JS 引擎**：[Duktape](https://duktape.org/)（MIT 许可，ANSI C，嵌入式）——JS 逻辑在后端执行，弹窗在前端显示

| 能力 | 支持情况 |
|---|---|
| HTML 标签 | 全部 HTML5 标签解析；`canvas`/`audio`/`iframe`/`svg` 完整支持 |
| CSS | 盒模型、Flex 布局、表格、浮动、定位、`:hover`/`:active` 伪类 |
| 视觉效果 | `border-radius`（四角独立+双半径）、`linear-gradient`/`radial-gradient`、`box-shadow`、`opacity` |
| 表单控件 | `input`(text/password/checkbox/radio/submit)、`select`、`button`、`textarea`——全部自绘 + 真实交互 |
| 键盘编辑 | ←/→/Home/End/Delete/Backspace，UTF-8 中文按字符处理 |
| 交互 | 链接/按钮点击 → C++ 通知回调；鼠标光标按 `cursor` 样式切换 |
| JS | 后端 Duktape 执行 `<script>`；`alert()`/`confirm()` 弹窗在前端窗口显示 |
| canvas | JS 绘图 API：`canvas_clear`/`canvas_fill_rect`/`canvas_draw_rect`/`canvas_draw_line`/`canvas_draw_circle`/`canvas_draw_text` |
| audio | Win32 MCI 播放本地 mp3/wav，自绘播放/暂停按钮 + 进度条 |
| iframe | 内嵌本地 HTML 子文档渲染 |
| svg | `rect`/`circle`/`line`/`text` 基本形状递归渲染 |

### 通用增强（继承自上游修复）

- 大量 use-after-free / 崩溃修复（右键菜单生命周期、消息路由判空等）
- XML 属性安全解析（越界读防护）
- WebView2 可选集成（`DUILIB_USE_WEBVIEW2`）

## 快速开始

### 编译

```bat
:: 静态库（Unicode / Win32）
msbuild DuiLib\DuiLib.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143 /m:1
copy /Y DuiLib\Lib\DuiLib.lib Lib\SRelease_Win32.lib

:: Demo
msbuild HtmlDemo\HtmlDemo.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143 /m:1
```

> 支持配置：`SRelease`/`SDebug`（静态库）、`Release`/`Debug`（动态库）× `Win32`/`x64`，另有多字节（A 后缀）变体。

### HTML 界面最小示例

```cpp
#include "UIlib.h"
using namespace DuiLib;

class CMyWindow : public CWindowWnd, public INotifyUI {
public:
    LPCTSTR GetWindowClassName() const override { return _T("HtmlWindow"); }
    void OnFinalMessage(HWND) override { delete this; }

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override {
        if (uMsg == WM_CREATE) {
            m_pm.Init(m_hWnd);
            // 创建 HtmlBox 作为根控件
            auto* pBox = new CHtmlBoxUI();
            pBox->SetManager(m_pm, NULL, true);
            m_pm.AttachDialog(pBox);

            // 内嵌 HTML（C++11 原始字符串，可直接从浏览器复制）
            pBox->SetHtml(LR"HTML(
                <style>
                    body { font-family: 微软雅黑; background: #EEF2F7; padding: 20px; }
                    .card {
                        background: #FFF; border-radius: 10px; padding: 24px;
                        box-shadow: 0 2px 10px rgba(0,0,0,.08);
                    }
                    .titlebar {
                        background: linear-gradient(180deg, #3A6BE8, #2D5BD7);
                        border-radius: 8px; color: #FFF; padding: 12px 20px;
                    }
                    input { width: 220px; height: 30px; border: 1px solid #C8CFDA; border-radius: 6px; }
                    button { background: #2D5BD7; color: #FFF; border: none;
                             border-radius: 6px; width: 90px; height: 32px; }
                    button:hover { background: #4E7BEC; }
                </style>
                <div class="titlebar"><b>DuiLib-X</b> — HTML 界面</div>
                <div class="card">
                    <h3>登录</h3>
                    <p><input type="text" name="user" placeholder="用户名"></p>
                    <p><input type="password" name="pwd"></p>
                    <button name="do_login" onclick="login()">登录</button>
                </div>
                <script>
                    function login() {
                        var u = get_value('user');
                        alert('你好, ' + u + '!');
                    }
                </script>
            )HTML");
            return 0;
        }
        LRESULT lRes = 0;
        if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes)) return lRes;
        return __super::HandleMessage(uMsg, wParam, lParam);
    }

    // 通知入口：HTML 交互 → C++ 后端逻辑
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_HTMLLINK) {
            // <a href> 被点击，msg.wParam = href
        }
        else if (msg.sType == DUI_MSGTYPE_HTMLFORMSUBMIT) {
            LPCTSTR name = (LPCTSTR)msg.wParam;   // 按钮/可点击元素的 name
            if (_tcsicmp(name, _T("js_close_window")) == 0)
                ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
        }
        else if (msg.sType == DUI_MSGTYPE_HTMLCHANGED) {
            // 表单值变化，msg.wParam = name
        }
    }

private:
    CPaintManagerUI m_pm;
};
```

### C++ 读写表单（前后端数据交互）

```cpp
CHtmlBoxUI* pBox = ...;
CDuiString user = pBox->GetValue(_T("user"));       // 读
pBox->SetValue(_T("user"), _T("李四"));              // 写
bool ok = pBox->IsChecked(_T("remember"));           // checkbox/radio
pBox->SetChecked(_T("remember"), true);
int idx = pBox->GetSelectedIndex(_T("city"));        // select
pBox->SetSelectedIndex(_T("city"), 2);

// 后端直接执行 JS
pBox->EvalJs(_T("alert('来自 C++ 的问候')"));
```

### JS 桥接 API（在后端 Duktape 中可调用）

| JS 函数 | 说明 |
|---|---|
| `alert(msg)` | 弹窗（在**前端** DuiLib 窗口显示） |
| `confirm(msg) → bool` | 询问框（前端显示，返回布尔） |
| `get_value(name)` | 读表单控件值 |
| `set_value(name, val)` | 写表单控件值 |
| `set_checked(name, bool)` | 勾选/取消 |
| `is_checked(name)` | 查询勾选状态 |
| `close_window()` | 关闭宿主窗口（经 C++ 通知） |
| `canvas_clear(id, color)` | 清空画布 |
| `canvas_fill_rect(id, x,y,w,h, color)` | 填充矩形 |
| `canvas_draw_rect(id, x,y,w,h, color, lineWidth)` | 描边矩形 |
| `canvas_draw_line(id, x1,y1,x2,y2, color, lineWidth)` | 画线 |
| `canvas_draw_circle(id, cx,cy,r, color, fill)` | 画圆 |
| `canvas_draw_text(id, text, x,y, color, size)` | 画文字 |

## 架构

```
DuiLib/Control/
├── UIHtmlBox.h/.cpp          DuiLib 控件外壳（SetHtml/表单API/JS API/通知）
└── litehtml/
    ├── container_win32.h/.cpp  litehtml document_container 桥接（字体/绘制/交互）
    ├── el_form.h/.cpp          自绘表单与媒体元素（replaced element 方式）
    │                           input/select/button/textarea/canvas/audio/iframe/svg
    └── js_bridge.h/.cpp        Duktape 后端 JS 引擎（弹窗转发前端）

3rd/
├── litehtml/                 BSD-3 · 排版引擎
└── duktape/                  MIT · JS 引擎（duktape.c 单文件）
```

**数据流**：HTML 字符串 → litehtml 解析/DOM → CSS 级联 → 布局 → `container_win32` 绘制回调 → GDI+ 像素。
交互：鼠标/键盘事件 → litehtml 元素（伪类/表单）→ 回调 → DuiLib `Notify` → 你的 C++ 业务代码。

## 目录结构

| 路径 | 内容 |
|---|---|
| `DuiLib/` | 库源码（静态/动态库，6+ 配置） |
| `DuiLib/Control/UIHtmlBox.*` | HtmlBox 控件对外 API |
| `DuiLib/Control/litehtml/` | litehtml 桥接 + 自绘元素 + JS 桥 |
| `3rd/litehtml/` | litehtml 源码 |
| `3rd/duktape/` | Duktape 源码 |
| `HtmlDemo/` | HTML 界面演示 + 无窗口自测 |
| `ControlDemo/` | 原生控件全量演示 |
| `Help/duilib使用文档.md` | 中文使用文档（含 HTML 引擎章节） |

## 文档

- [中文文档：Help/duilib使用文档.md](Help/duilib使用文档.md) — 第 14 章为 HTML/CSS 引擎专章
- 控件属性速查：`Help/Duilib属性列表.html`

## 许可证

- DuiLib-X：遵循上游 DuiLib 许可（免费开源）
- [litehtml](https://github.com/litehtml/litehtml)：BSD-3
- [Duktape](https://duktape.org/)：MIT
