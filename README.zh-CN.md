# DuiLib-X

[English](README.md) | [中文](README.zh-CN.md)

**DuiLib-X** is an enhanced fork of DuiLib, the Windows DirectUI library. On top of the original library it adds **HTML/CSS UI rendering**: write your UI in HTML/CSS on the front end and your logic in C++ on the back end — with no browser engine involved.

## Highlights

### HTML/CSS Rendering Engine (powered by litehtml)

- **Layout engine**: [litehtml](https://github.com/litehtml/litehtml) (BSD-3, C++, not a browser engine)
- **Paint layer**: GDI+ (anti-aliasing, rounded paths, gradients, real alpha blending)
- **JS engine**: [Duktape](https://duktape.org/) (MIT, ANSI C, embeddable) — JS runs on the back end, dialogs pop up on the front end

| Capability | Support |
|---|---|
| HTML tags | Full HTML5 tag parsing; `canvas`/`audio`/`iframe`/`svg` fully supported |
| CSS | Box model, Flexbox, tables, floats, positioning, `:hover`/`:active` pseudo-classes |
| Visual effects | `border-radius` (4 independent corners + dual radii), `linear-gradient`/`radial-gradient`, `box-shadow`, `opacity` |
| Form controls | `input` (text/password/checkbox/radio/submit), `select`, `button`, `textarea` — all custom-drawn with real interaction |
| Keyboard editing | ←/→/Home/End/Delete/Backspace, UTF-8 Chinese handled per character |
| Interaction | Link/button clicks → C++ notify callbacks; mouse cursor follows the `cursor` style |
| JS | `<script>` executed by back-end Duktape; `alert()`/`confirm()` dialogs shown in the front-end window |
| canvas | JS drawing API: `canvas_clear`/`canvas_fill_rect`/`canvas_draw_rect`/`canvas_draw_line`/`canvas_draw_circle`/`canvas_draw_text` |
| audio | Win32 MCI playback of local mp3/wav, custom-drawn play/pause button + progress bar |
| iframe | Embedded rendering of local HTML sub-documents |
| svg | Recursive rendering of basic shapes: `rect`/`circle`/`line`/`text` |

### General Enhancements (inherited fixes)

- Numerous use-after-free / crash fixes (context-menu lifetime, message-routing null guards, etc.)
- Safe XML attribute parsing (out-of-bounds read protection)
- Optional WebView2 integration (`DUILIB_USE_WEBVIEW2`)

## Quick Start

### Build

```bat
:: Static library (Unicode / Win32)
msbuild DuiLib\DuiLib.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143 /m:1
copy /Y DuiLib\Lib\DuiLib.lib Lib\SRelease_Win32.lib

:: Demo
msbuild HtmlDemo\HtmlDemo.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143 /m:1
```

> Configurations: `SRelease`/`SDebug` (static), `Release`/`Debug` (DLL) × `Win32`/`x64`, plus multibyte variants (A suffix).

### Minimal HTML UI Example

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
            // Use an HtmlBox as the root control
            auto* pBox = new CHtmlBoxUI();
            pBox->SetManager(m_pm, NULL, true);
            m_pm.AttachDialog(pBox);

            // Embedded HTML (C++11 raw string — paste straight from a browser)
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
                <div class="titlebar"><b>DuiLib-X</b> — HTML UI</div>
                <div class="card">
                    <h3>Sign in</h3>
                    <p><input type="text" name="user" placeholder="username"></p>
                    <p><input type="password" name="pwd"></p>
                    <button name="do_login" onclick="login()">Sign in</button>
                </div>
                <script>
                    function login() {
                        var u = get_value('user');
                        alert('Hello, ' + u + '!');
                    }
                </script>
            )HTML");
            return 0;
        }
        LRESULT lRes = 0;
        if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes)) return lRes;
        return __super::HandleMessage(uMsg, wParam, lParam);
    }

    // Notification entry: HTML interaction → C++ back-end logic
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_HTMLLINK) {
            // an <a href> was clicked; msg.wParam = href
        }
        else if (msg.sType == DUI_MSGTYPE_HTMLFORMSUBMIT) {
            LPCTSTR name = (LPCTSTR)msg.wParam;   // name of the clicked button/element
            if (_tcsicmp(name, _T("js_close_window")) == 0)
                ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
        }
        else if (msg.sType == DUI_MSGTYPE_HTMLCHANGED) {
            // a form value changed; msg.wParam = name
        }
    }

private:
    CPaintManagerUI m_pm;
};
```

### Reading/Writing Forms from C++ (front/back-end data exchange)

```cpp
CHtmlBoxUI* pBox = ...;
CDuiString user = pBox->GetValue(_T("user"));       // read
pBox->SetValue(_T("user"), _T("Li Si"));            // write
bool ok = pBox->IsChecked(_T("remember"));           // checkbox/radio
pBox->SetChecked(_T("remember"), true);
int idx = pBox->GetSelectedIndex(_T("city"));        // select
pBox->SetSelectedIndex(_T("city"), 2);

// Run JS directly from the back end
pBox->EvalJs(_T("alert('Hello from C++')"));
```

### JS Bridge API (callable inside back-end Duktape)

| JS function | Description |
|---|---|
| `alert(msg)` | Dialog (rendered in the **front-end** DuiLib window) |
| `confirm(msg) → bool` | Confirmation dialog (front-end, returns boolean) |
| `get_value(name)` | Read a form control's value |
| `set_value(name, val)` | Write a form control's value |
| `set_checked(name, bool)` | Check/uncheck |
| `is_checked(name)` | Query checked state |
| `close_window()` | Close the host window (via C++ notify) |
| `canvas_clear(id, color)` | Clear the canvas |
| `canvas_fill_rect(id, x,y,w,h, color)` | Fill a rectangle |
| `canvas_draw_rect(id, x,y,w,h, color, lineWidth)` | Stroke a rectangle |
| `canvas_draw_line(id, x1,y1,x2,y2, color, lineWidth)` | Draw a line |
| `canvas_draw_circle(id, cx,cy,r, color, fill)` | Draw a circle |
| `canvas_draw_text(id, text, x,y, color, size)` | Draw text |

## Architecture

```
DuiLib/Control/
├── UIHtmlBox.h/.cpp          DuiLib control shell (SetHtml / form API / JS API / notifications)
└── litehtml/
    ├── container_win32.h/.cpp  litehtml document_container bridge (fonts / painting / interaction)
    ├── el_form.h/.cpp          Custom-drawn form & media elements (replaced-element approach)
    │                           input/select/button/textarea/canvas/audio/iframe/svg
    └── js_bridge.h/.cpp        Duktape back-end JS engine (dialogs forwarded to the front end)

3rd/
├── litehtml/                 BSD-3 · layout engine
└── duktape/                  MIT · JS engine (single-file duktape.c)
```

**Data flow**: HTML string → litehtml parse/DOM → CSS cascade → layout → `container_win32` paint callbacks → GDI+ pixels.
Interaction: mouse/keyboard events → litehtml elements (pseudo-classes/forms) → callbacks → DuiLib `Notify` → your C++ business code.

## Repository Layout

| Path | Contents |
|---|---|
| `DuiLib/` | Library sources (static/DLL, 6+ configurations) |
| `DuiLib/Control/UIHtmlBox.*` | Public HtmlBox control API |
| `DuiLib/Control/litehtml/` | litehtml bridge + custom-drawn elements + JS bridge |
| `3rd/litehtml/` | litehtml sources |
| `3rd/duktape/` | Duktape sources |
| `HtmlDemo/` | HTML UI demo + windowless self-tests |
| `ControlDemo/` | Full native-control showcase |
| `Help/duilib使用文档.md` | Chinese documentation (includes the HTML engine chapter) |

## Documentation

- Chinese docs: [Help/duilib使用文档.md](Help/duilib使用文档.md) — Chapter 14 covers the HTML/CSS engine
- Control property reference: `Help/Duilib属性列表.html`

## License

- DuiLib-X: follows the upstream DuiLib license (free and open source)
- [litehtml](https://github.com/litehtml/litehtml): BSD-3
- [Duktape](https://duktape.org/): MIT
