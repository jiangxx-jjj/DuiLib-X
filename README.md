# DuiLib-X

**一个深度加固的 DuiLib 分支：60+ bug 修复 + 自研 HTML5/CSS3 渲染引擎，零浏览器内核依赖。**

DuiLib-X 基于 DuiLib_Ultimate 做了全面修复与增强：

- 修复原版 **60+ 个 bug**（崩溃 / 内存破坏 / COM 泄漏 / use-after-free）
- 自研 **HTML5 + CSS3 渲染引擎**（约 6000 行 C++），用 HTML 写界面，不需要 WebView、不需要 litehtml、不需要任何第三方排版库
- 圆角无边框窗口、HTML 自绘标题栏（最小化/最大化/关闭全部用 HTML 实现）

## 特性

### 自研 HTML/CSS 引擎

| 模块 | 能力 |
|---|---|
| HTML 解析 | 自研容错解析器，支持 HTML5 全部常用标签、自动闭合、实体解码 |
| CSS 解析 | 自研解析器，支持标签/类/ID/属性/后代/子/兄弟选择器 + 伪类 + 级联 + 特异性排序 |
| 布局 | Block 流式、Inline 流式（自动换行）、Flex、Table、绝对定位、浮动、列表项 |
| 绘制 | GDI+ 抗锯齿：**圆角、线性/径向渐变、盒阴影、文字阴影、透明度** |
| 表单 | input(text/password/checkbox/radio/hidden/submit)、select、button、textarea —— **全部自绘 + 真实交互** |

### 关键 CSS3 能力（自研实现，非第三方）

- `border-radius` —— 四角独立、水平/垂直双半径，背景和边框都按圆角裁剪
- `linear-gradient()` / `radial-gradient()` —— 任意角度、多色标、自动位置分配
- `box-shadow` / `text-shadow` —— 32 位 DIB + AlphaBlend 实现真正的半透明模糊
- `opacity` —— 逐层累积传递到子元素
- `flex` 布局 —— direction / wrap / justify-content / align-items / grow / gap

### 表单控件（真实可用）

- 文本框：真实键盘输入、退格/删除、**UTF-8 按字符而非字节处理**（中英文混排正确）
- 密码框：内容显示为 `*`
- 复选框：白底方框 + 蓝色对勾，点击切换
- 单选框：白底圆圈 + 蓝色圆点，同 `name` 互斥
- 下拉框：当前项文字 + 三角箭头，点击循环切换
- 按钮：触发 `DUI_MSGTYPE_HTMLFORMSUBMIT` 回调

### 核心修复（60+）

- 右键菜单生命周期 use-after-free 全链修复（卡死/闪退根因）
- 窗口过程 `OnFinalMessage(delete this)` 后写已释放对象 → 堆破坏
- GDI/GDI+ 对象泄漏、COM 引用计数错误、迭代器失效
- 属性解析越界读（畸形 XML 不再崩溃）
- PDB 并发写冲突、GetMessage -1 忙死循环
- 详见 [BUGFIX_PLAN.md](BUGFIX_PLAN.md)

## 快速开始

### 编译

```bat
:: 1. 编译 DuiLib 主库
msbuild DuiLib\DuiLib.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143

:: 2. ★关键★ 同步静态库（否则 msbuild 判定 exe 最新会跳过链接，改了源码"毫无效果"）
copy /Y DuiLib\Lib\DuiLib.lib Lib\SRelease_Win32.lib

:: 3. 编译示例
msbuild ControlDemo\ControlDemo.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143
msbuild HtmlDemo\HtmlDemo.vcxproj /p:Configuration=SRelease /p:Platform=Win32 /p:PlatformToolset=v143
```

要求：Visual Studio 2022（v143 工具集）、Windows SDK 10.0.19041.0

> 六个配置（`SRelease` / `SDebug` / `Release` × `Win32` / `x64`）均已验证编译通过。
> `Release` 为 DLL 配置（`UILIB_EXPORTS`），其余为静态库配置。
>
> **不再需要编译 litehtml** —— 引擎已完全自研，`3rd/litehtml` 仅作历史保留。

### 自测

```bat
msbuild HtmlDemo\HtmlFormTest.vcxproj /p:Configuration=SRelease /p:Platform=Win32
HtmlDemo\bin\HtmlFormTest.exe
:: 输出 25 项断言（表单查找/读写/勾选/中英文输入/退格），全 PASS
```

### 运行示例

- `HtmlDemo/bin/HtmlDemo.exe` — 纯 HTML 界面演示（圆角无边框窗口 + HTML 自绘标题栏 + 表单）
- `ControlDemo/bin/demo/ControlDemoS.exe` — 控件全家桶（8 个 Tab 页含 HTML 界面演示）

### 用 HTML 写界面

**推荐用 C++11 原始字符串内嵌 HTML** —— 不用转义引号、不用逐行 `_T()`，可直接从浏览器复制：

```cpp
#include "UIlib.h"
using namespace DuiLib;

// LR"( ... )" = 宽字符原始字符串，直接赋给 LPCTSTR
static LPCTSTR kPage = LR"HTML(<!DOCTYPE html>
<html>
<head>
<style>
    body { font-family: 微软雅黑; background: #EEF2F7; }
    .card {
        background: #FFFFFF;
        border: 1px solid #DCE3ED;
        border-radius: 10px;
        box-shadow: 0 2px 10px rgba(45, 91, 215, 0.08);
        padding: 16px 18px;
        margin: 14px 16px;
    }
    .titlebar {
        background: linear-gradient(180deg, #3A6BE8 0%, #2D5BD7 100%);
        height: 52px;
        border-radius: 10px 10px 0 0;
    }
    input[type=text] { width: 260px; height: 32px; border: 1px solid #C8CFDA; border-radius: 6px; }
</style>
</head>
<body>
    <div class="titlebar"><span style="color:#FFF;line-height:52px;padding-left:20px;">DuiLib-X</span></div>
    <div class="card">
        <h3 style="color:#2D5BD7;">登录</h3>
        <div><label>用户名：</label><input type="text" name="user" value="张三"/></div>
        <div><label>密码：</label><input type="password" name="pwd"/></div>
        <div><button name="do_login">登 录</button></div>
    </div>
</body>
</html>
)HTML";

// 创建控件
CHtmlBoxUI* pBox = new CHtmlBoxUI();
pBox->SetName(_T("htmlBox"));
pBox->SetBkColor(0xFFEEF2F7);
pBox->SetManager(m_pm, NULL, true);
m_pm->AttachDialog(pBox);
pBox->SetHtml(kPage);

// 事件与数据（C++ 后端逻辑）
void Notify(TNotifyUI& msg) override {
    if (msg.sType == DUI_MSGTYPE_HTMLFORMSUBMIT) {           // 按钮被点击
        LPCTSTR pName = (LPCTSTR)msg.wParam;                  // 按钮的 name
        CDuiString user = pBox->GetValue(_T("user"));         // 读表单字段
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

XML 写法：

```xml
<HtmlBox name="htmlBox" bkcolor="0xFFEEF2F7" html="page.html" />
```

## 支持的 HTML/CSS

### HTML 标签

| 类别 | 标签 |
|---|---|
| 结构 | `html` `head` `body` `title` `meta` `style` |
| 块级 | `div` `section` `article` `aside` `header` `footer` `nav` `main` `figure` `figcaption` `p` `h1`~`h6` `blockquote` `pre` `hr` `address` `dl` `dt` `dd` |
| 行内 | `span` `a` `b` `strong` `i` `em` `u` `s` `del` `ins` `small` `big` `mark` `sub` `sup` `code` `kbd` `samp` `var` `cite` `abbr` `q` `time` `tt` `font` |
| 列表 | `ul` `ol` `li` |
| 表格 | `table` `thead` `tbody` `tfoot` `tr` `td` `th` `caption`（支持 `colspan`） |
| 表单 | `input` `select` `option` `button` `textarea` `label` `form` `fieldset` `legend` |
| 媒体 | `img`（本地图片，支持 DuiLib zip 资源包） |
| 不支持 | `canvas` `video` `audio` `iframe` `svg` |

### CSS3 属性

| 类别 | 属性 |
|---|---|
| 布局 | `display`(block/inline/inline-block/flex/table/list-item/none) `position`(static/relative/absolute/fixed) `top/right/bottom/left` `float` `clear` `z-index` `box-sizing` `overflow` |
| 尺寸 | `width` `height` `min/max-width` `min/max-height`（px/%/em/rem/pt/vw/vh/auto） |
| 盒模型 | `margin` `padding` `border` `border-*` `border-radius`（四角 + 水平/垂直双半径） |
| 背景 | `background-color` `background-image`（url / linear-gradient / radial-gradient）`background` |
| 文字 | `color` `font-size` `font-family` `font-weight` `font-style` `font` `line-height` `text-align` `text-decoration` `text-indent` `letter-spacing` `white-space` `vertical-align` |
| **CSS3 效果** | **`opacity`** **`box-shadow`** **`text-shadow`** **`filter`** |
| Flex | `flex-direction` `flex-wrap` `justify-content` `align-items` `align-self` `flex-grow` `flex-shrink` `flex-basis` `flex` `gap` |
| 列表表格 | `list-style-type` `list-style-position` `border-collapse` `border-spacing` |
| 其它 | `cursor` `visibility` |

### CSS 选择器

`tag` · `.class` · `#id` · `*` · `[attr]` · `[attr=val]` · `[attr~=]` · `[attr^=]` · `[attr$=]` · `[attr*=]`
· 后代（空格）· 子（`>`）· 相邻兄弟（`+`）· 一般兄弟（`~`）
· 伪类 `:hover` `:active` `:first-child` `:last-child` `:nth-child(n)` `:not()` `:link` `:checked` `:disabled` `:root` `:empty`

### 表单控件事件

| 消息 | 触发时机 | `wParam` |
|---|---|---|
| `DUI_MSGTYPE_HTMLLINK` | `<a href>` 被点击 | 链接地址 |
| `DUI_MSGTYPE_HTMLFORMSUBMIT` | `<button>` / 带 `name` 的元素被点击 | 元素 `name` |
| `DUI_MSGTYPE_HTMLCHANGED` | 任意表单字段值变化 | 元素 `name` |

### 表单数据 API

```cpp
pBox->GetValue(_T("user"));            // 读 input/select/textarea
pBox->SetValue(_T("user"), _T("李四"));
pBox->IsChecked(_T("remember"));       // checkbox/radio
pBox->SetChecked(_T("remember"), false);
pBox->GetSelectedIndex(_T("city"));    // select
pBox->SetSelectedIndex(_T("city"), 2);
```

完整属性列表见 [Help/Duilib属性列表.html](Help/Duilib%E5%B1%9E%E6%80%A7%E5%88%97%E8%A1%A8.html)，
详细使用文档见 [Help/duilib使用文档.md](Help/duilib%E4%BD%BF%E7%94%A8%E6%96%87%E6%A1%A3.md)
（第 14 章专门讲 HTML 引擎）。

## 目录结构

```
DuiLib/                    核心库源码
  Control/HtmlEngine/      自研 HTML/CSS 引擎
    HtmlTypes.h/.cpp         基础类型（颜色/长度/枚举/阴影/渐变/边框）
    HtmlDom.h/.cpp           DOM 节点树 + 计算样式
    HtmlParser.h/.cpp        HTML5 容错解析器
    HtmlCss.h/.cpp           CSS3 解析 + 选择器 + 级联
    HtmlLayout.h/.cpp        布局引擎（Block/Inline/Flex/Table/绝对定位）
    HtmlPainter.h/.cpp       GDI+ 绘制（圆角/渐变/阴影/文字/表单）
    HtmlDocument.h/.cpp      引擎门面
  Control/UIHtmlBox.h/.cpp  HtmlBox 控件（对外 API）
HtmlDemo/                  纯 HTML 界面演示
  HtmlDemo.cpp               圆角无边框窗口 + HTML 自绘标题栏
  HtmlFormTest.cpp/.vcxproj  表单链路无窗口自测（25 项断言）
ControlDemo/               控件全家桶演示
Help/                      使用文档与属性列表
BUGFIX_PLAN.md             完整修复记录
```

## 许可证

- DuiLib: [duilib license](Help/duilib%20license.txt) / [directui license](Help/directui%20license.txt)

## 致谢

- [DuiLib](https://github.com/duilib/duilib) — 原始项目
- [DuiLib_Ultimate](https://github.com/qdtroy/DuiLib_Ultimate) — 增强版上游
