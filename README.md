# DuiLib-X

**A hardened fork of DuiLib with 60+ bug fixes and built-in lightweight HTML/CSS rendering — no browser engine required.**

DuiLib-X 是 DuiLib_Ultimate 的深度修复与增强版：
修复原版 60+ 个 bug（崩溃/内存破坏/COM泄漏等），
并集成 [litehtml](https://github.com/litehtml/litehtml)（BSD-3）
实现**零浏览器内核依赖的 HTML/CSS 界面渲染**。

## 特性

### 核心修复（60+）
- 右键菜单生命周期 use-after-free 全链修复（卡死/闪退根因）
- 窗口过程 `OnFinalMessage(delete this)` 后写已释放对象 → 堆破坏
- litehtml 集成后的 HTML 界面渲染
- GDI/GDI+ 对象泄漏、COM 引用计数错误、迭代器失效
- 属性解析越界读（畸形 XML 不再崩溃）
- 详见 [BUGFIX_PLAN.md](BUGFIX_PLAN.md)

### HTML 界面（HtmlBox 控件）
- 用 HTML/CSS 写界面，litehtml 排版，DuiLib 原生渲染
- 支持 CSS2/CSS3 选择器、级联、盒模型、`<style>` 块与外部样式表
- **表单控件原生实现**：input(text/password/checkbox/radio/hidden)、select、button、textarea
  - 文本输入框支持真实键盘输入、光标移动、退格/删除（中英文混排正确）
  - checkbox/radio 可点击切换、select 支持选项切换、button 触发提交回调
- 零 JS 依赖，事件通过 C++ 回调（类似 WebView 的事件模型）
- 数据双向交互：`GetValue / SetValue / IsChecked / SetChecked`
- 垂直滚动、DPI 自适应、`<a href>` 链接点击回调

### 稳定性增强
- 全部窗口/菜单销毁路径的生命周期审计
- PDB 并发写冲突修复、GetMessage -1 忙死循环修复
- 右键菜单 `CMenuWnd` 全链路重构（同步销毁 + 快照广播）

## 快速开始

### 编译

```bat
:: 1. 编译 litehtml（HTML/CSS 引擎）
msbuild 3rd\litehtml\litehtml.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143

:: 2. 编译 DuiLib 主库
msbuild DuiLib\DuiLib.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143

:: 3. 编译示例
msbuild ControlDemo\ControlDemo.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143
msbuild HtmlDemo\HtmlDemo.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143
```

要求：Visual Studio 2022（v143 工具集）、Windows SDK 10.0.19041.0

### 运行示例

- `ControlDemo/bin/demo/ControlDemoS.exe` — 控件全家桶（8 个 Tab 页含 HTML 界面演示）
- `HtmlDemo/bin/HtmlDemo.exe` — 纯 HTML 界面演示（单窗口全屏 HTML）

### 用 HTML 写界面

```cpp
// 界面直接用 HTML 描述（也支持 SetHtmlFromFile 从文件加载）
LPCTSTR html =
_T("<style>.card { background:#FFF; border:1px solid #DDD; padding:14px; }</style>")
_T("<div class='card'>")
_T("  <div class='row'><label>用户名：</label><input type='text' name='user' value='张三'/></div>")
_T("  <div class='row'><label>密码：</label><input type='password' name='pwd'/></div>")
_T("  <div class='row'><label>城市：</label>")
_T("    <select name='city'><option>北京</option><option selected>上海</option></select></div>")
_T("  <div class='row'><input type='checkbox' name='remember' checked/><label>记住密码</label></div>")
_T("  <button name='do_login'>登 录</button>")
_T("</div>");

CHtmlBoxUI* pBox = new CHtmlBoxUI();
pBox->SetHtml(html);

// 事件与数据（C++ 后端逻辑）
void Notify(TNotifyUI& msg) override {
    if (msg.sType == DUI_MSGTYPE_HTMLFORMSUBMIT) {           // 按钮被点击
        CDuiString user = pBox->GetValue(_T("user"));         // 读表单字段
        CDuiString city = pBox->GetValue(_T("city"));
        bool remember   = pBox->IsChecked(_T("remember"));
        pBox->SetValue(_T("user"), _T("欢迎回来"));            // 回写界面
    }
    else if (msg.sType == DUI_MSGTYPE_HTMLCHANGED) {          // 任意字段值变化
        // msg.wParam 为发生变化的字段 name
    }
    else if (msg.sType == DUI_MSGTYPE_HTMLLINK) {             // <a href> 被点击
        // msg.wParam 为链接地址
    }
}
```

## 支持的 HTML/CSS

| 类别 | 支持 |
|---|---|
| 文本 | h1-h6/p/b/i/u/font/br/span，颜色字号字重装饰 |
| 链接 | `<a href>` 点击事件回调 |
| 图片 | 本地图片（含 DuiLib zip 资源包） |
| 表单 | input(text/password/checkbox/radio/hidden/submit)、select、button、textarea |
| 布局 | div 嵌套/margin/padding/border/table/hr/display:inline-block |
| CSS | 选择器/级联/继承/style 块/外部 css/@import/伪类(:hover :active :link) |
| 不支持 | JavaScript、CSS 动画、canvas/video/iframe |

### 表单控件事件

| 消息 | 触发时机 | wParam |
|---|---|---|
| `DUI_MSGTYPE_HTMLLINK` | `<a href>` 被点击 | 链接地址 |
| `DUI_MSGTYPE_HTMLFORMSUBMIT` | `<button>` / `<input type=submit>` 被点击 | 控件 `name` |
| `DUI_MSGTYPE_HTMLCHANGED` | 任意表单字段值变化 | 控件 `name` |

完整属性列表见 [Help/Duilib属性列表.html](Help/Duilib%E5%B1%9E%E6%80%A7%E5%88%97%E8%A1%A8.html)，
使用文档见 [Help/duilib使用文档.md](Help/duilib%E4%BD%BF%E7%94%A8%E6%96%87%E6%A1%A3.md)。

## 目录结构

```
DuiLib/        核心库源码（Core/Control/Layout/Utils）
  Control/UIDuiHtmlContainer.h/.cpp   litehtml 绘制容器（document_container 实现）
  Control/UIDuiHtmlForm.h/.cpp        表单控件元素（input/select/button/textarea）
  Control/UIHtmlBox.h/.cpp            HtmlBox 控件（对外 API）
HtmlDemo/      纯 HTML 界面演示
  HtmlFormTest.cpp/.vcxproj           表单链路无窗口自测（25 项断言）
ControlDemo/   控件全家桶演示
3rd/litehtml/  litehtml 引擎（BSD-3）
Help/          使用文档与属性列表
BUGFIX_PLAN.md 完整修复记录
```

## 许可证

- DuiLib: [duilib license](Help/duilib%20license.txt) / [directui license](Help/directui%20license.txt)
- litehtml: BSD-3-Clause（[3rd/litehtml/LICENSE](3rd/litehtml/LICENSE)）

## 致谢

- [DuiLib](https://github.com/duilib/duilib) — 原始项目
- [DuiLib_Ultimate](https://github.com/qdtroy/DuiLib_Ultimate) — 增强版上游
- [litehtml](https://github.com/litehtml/litehtml) — HTML/CSS 排版引擎