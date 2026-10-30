# DuiLib 使用文档

> 基于 **DuiLib_Ultimate**（qdtroy 增强版）编写，配套示例工程 `ControlDemo/`，
> 示例覆盖了 `Help/Duilib属性列表.html` 中的**全部控件**，可与本文档对照学习。

---

## 目录

1. [DuiLib 是什么](#1-duilib-是什么)
2. [环境搭建与编译](#2-环境搭建与编译)
3. [第一个 DuiLib 程序](#3第一个-duilib-程序)
4. [窗口：XML 根节点 Window](#4窗口xml-根节点-window)
5. [XML 布局系统](#5xml-布局系统)
6. [控件总览与常用属性](#6控件总览与常用属性)
7. [消息处理与事件响应](#7消息处理与事件响应)
8. [无边框窗口、圆角与自绘标题栏](#8无边框窗口圆角与自绘标题栏)
9. [资源管理：路径、zip 包与换肤](#9资源管理路径zip-包与换肤)
10. [高 DPI、HSL 调色与阴影](#10高-dpihsl-调色与阴影)
11. [常见问题与调试技巧](#11常见问题与调试技巧)
12. [工程实践建议](#12工程实践建议)
13. [实战经验：崩溃案例分析](#13实战经验崩溃案例分析) ← 本仓库实战修复总结
14. [附录 A：示例工程说明](#附录-a示例工程说明)
15. [附录 B：XML 属性书写规范速记](#附录-bxml-属性书写规范速记)

---

## 1. DuiLib 是什么

DuiLib 是 Windows 平台的 **DirectUI** 界面库。传统 Win32 程序里每个按钮都是真实系统窗口；
DirectUI 则是**整个窗口只有 1 个 HWND**，所有控件都是逻辑对象，由库自行完成：

- **布局计算**（纵横/平铺/选项卡/树形）
- **消息路由**（鼠标/键盘消息按坐标分发给命中的控件）
- **自绘渲染**（GDI / GDI+，全部控件无真实句柄）

**收益**：界面完全自绘 → 任意皮肤；控件数量不受句柄限制；XML 描述界面 → 美工和程序分离。

**技术栈**：纯 C++/Win32，无 .NET 依赖；支持 Unicode/多字节、静态库/动态库。

---

## 2. 环境搭建与编译

### 2.1 编译库

```bat
:: 静态库（Unicode）—— Demo 默认方式，exe 无需带 DLL
msbuild DuiLib\DuiLib.vcxproj /p:Configuration=SRelease /p:Platform=Win32 ^
        /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.19041.0 ^
        /p:OutDir=..\Lib\ /p:TargetName=SRelease_Win32

:: 动态库（Unicode）—— 多个 exe 共享一个 DLL
msbuild DuiLib\DuiLib.vcxproj /p:Configuration=Release /p:Platform=Win32 ...
```

| 配置 | 含义 |
|---|---|
| `SRelease` / `SDebug` | 静态库，Unicode |
| `Release` / `Debug` | 动态库，Unicode |
| `SReleaseA` / `ReleaseA` | 静态/动态库，多字节 |

产物位置：`Lib\SRelease_Win32.lib`（静态）、`DuiLib\Lib\DuiLib.lib + .dll`（动态导入库）。

### 2.2 新建自己的工程

以 **静态库 + v143** 为例（示例工程 `ControlDemo/ControlDemo.vcxproj` 可直接抄）：

```
包含目录:  DuiLib;DuiLib\Core;DuiLib\Control;DuiLib\Layout;DuiLib\Utils
预处理:    UILIB_STATIC        ← 静态链接时必须定义（与库一致）
运行库:    MultiThreaded(/MT)  ← 必须与 SRelease 库一致（动态库则 /MD）
链接库:    SRelease_Win32.lib
```

> ⚠️ **运行库不一致**是新手最常见的链接错误来源（LNK2038/LNK2019）。
> 静态库配 `/MT`，动态库配 `/MD`。

---

## 3. 第一个 DuiLib 程序

```cpp
#include "UIlib.h"
using namespace DuiLib;

// 窗口框架：继承 CWindowWnd + INotifyUI（事件）+ IDialogBuilderCallback（自定义控件）
class CMyWindow : public CWindowWnd, public INotifyUI
{
public:
    LPCTSTR GetWindowClassName() const override { return _T("MyWindow"); }
    void OnFinalMessage(HWND) override { delete this; }   // 必须自删，标准套路

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        if (uMsg == WM_CREATE)
        {
            m_pm.Init(m_hWnd);
            // 资源路径：exe 所在目录的 skin 文件夹
            CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath() + _T("skin"));

            CDialogBuilder builder;
            CControlUI* pRoot = builder.Create(_T("main.xml"), (UINT)0, NULL, &m_pm);
            m_pm.AttachDialog(pRoot);     // 挂载控件树
            m_pm.AddNotifier(this);       // 注册通知处理
            return 0;
        }
        if (uMsg == WM_DESTROY) { PostQuitMessage(0); return 0; }

        LRESULT lRes = 0;
        if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes))
            return lRes;                              // 控件消息已处理
        return __super::HandleMessage(uMsg, wParam, lParam);  // 其余交还系统
    }

    void Notify(TNotifyUI& msg) override
    {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btnHello"))
            MessageBox(m_hWnd, _T("Hello DuiLib!"), _T("提示"), MB_OK);
    }

private:
    CPaintManagerUI m_pm;
};

int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR, int nShow)
{
    ::CoInitialize(NULL);
    CPaintManagerUI::SetInstance(hInst);

    CMyWindow* pWin = new CMyWindow;
    pWin->Create(NULL, _T("标题"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
    pWin->CenterWindow();
    pWin->ShowWindow();

    CPaintManagerUI::MessageLoop();     // 库自带消息循环
    ::CoUninitialize();
    return 0;
}
```

**最小 skin/main.xml**：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Window size="400,300" caption="0,0,0,32">
  <VerticalLayout bkcolor="0xFFFFFFFF" inset="10,10,10,10">
    <Button name="btnHello" text="点我" width="120" height="32"/>
  </VerticalLayout>
</Window>
```

三要素：**窗口类**（HandleMessage 路由）+ **XML 皮肤** + **Notify 响应事件**。

---

## 4. 窗口：XML 根节点 Window

`<Window>` 属性（XML 解析见 `UIDlgBuilder.cpp`）：

| 属性 | 示例 | 说明 |
|---|---|---|
| `size` | `1180,760` | 初始尺寸（会按 DPI 缩放） |
| `mininfo` / `maxinfo` | `960,640` | 最小/最大尺寸 |
| `sizebox` | `4,4,6,6` | **四周可拖拽缩放的边距**（无边框窗口必需） |
| `caption` | `0,0,0,32` | **标题栏拖动区高度**（区域内静态控件上按住可拖动窗口） |
| `roundcorner` | `8,8` | 窗口圆角（WM_SIZE 时建立 RoundRect RGN） |
| `alpha` | `255` | 整窗透明度 0~255 |
| `disabledfontcolor` 等 | 色值 | 各类默认颜色 |
| `showdirty` | `true` | 显示重绘区域（调试布局利器） |

Window 的**子节点**中还可写全局定义（放在最前面）：

```xml
<Window size="800,600">
  <!-- 全局字体：控件用 font="0" 引用 -->
  <Font id="0" name="微软雅黑" size="14"/>
  <!-- 控件默认属性：所有 Button 自动带上 -->
  <Default name="Button" value="font=&quot;0&quot; hotbkcolor=&quot;0xFF3A7AFF&quot;"/>
  <!-- 样式表：控件用 style="btnBlue" 应用 -->
  <Style name="btnBlue" value="bkcolor=&quot;0xFF3A7AFF&quot; textcolor=&quot;0xFFFFFFFF&quot;"/>
  <!-- 外部 XML 嵌入 / 字体文件加载 -->
  <Include source="common.xml" count="1"/>
  <Import fontfile="iconfont.ttf"/>
  ...
</Window>
```

---

## 5. XML 布局系统

### 5.1 三大线性容器

```xml
<!-- 纵向排列 -->
<VerticalLayout width="200" inset="8,10,8,8" childpadding="6" bkcolor="0xFFFFFFFF">
  ...子控件按高度依次向下排...
</VerticalLayout>

<!-- 横向排列 -->
<HorizontalLayout height="40" childpadding="8">
  <Button text="A" width="80"/>   <!-- 固定宽 -->
  <Button text="B"/>              <!-- 无 width/height → 拉伸占满剩余空间 -->
  <Button text="C" width="80"/>
</HorizontalLayout>
```

要点：
- **`inset="左,上,右,下"`**：容器内边距（等价 CSS padding）
- **`childpadding`**：子控件间距
- 子控件**不设 width/height 则自动拉伸**；设置了则固定
- `float="true"`：脱离布局流，用 `pos="x,y,w,h"` 绝对定位

### 5.2 TileLayout 平铺

```xml
<TileLayout columns="5" itemsize="0,64" childpadding="8">
  <!-- columns=列数；itemsize="宽,高" 每格大小（宽 0 表示按列均分） -->
  <Label text="1"/> ... 
</TileLayout>
```

### 5.3 TabLayout 选项卡

```xml
<TabLayout name="tabMain" selected="0">
  <VerticalLayout>第 1 页内容</VerticalLayout>
  <VerticalLayout>第 2 页内容</VerticalLayout>
</TabLayout>
```

- 代码切换：`pTab->SelectItem(1)`
- **零代码联动**：按钮加 `bindtablayoutname="tabMain" bindtabindex="1"`，点击自动切页
- 动画切换：换成 `AnimationTabLayout`，加 `animation_direction="vertical|horizontal"`

### 5.4 ChildLayout 子 XML 嵌入

```xml
<ChildLayout xmlfile="child_panel.xml" height="54"/>
```

把独立 XML 解析为子控件树嵌入，适合模块化拆分皮肤。

### 5.5 滚动

任何容器加 `vscrollbar="true"` / `hscrollbar="true"` 即出现滚动条：

```xml
<VerticalLayout vscrollbar="true" inset="8,4,8,4">
  <Label text="内容超出容器高度就会出现滚动条" height="2000"/>
</VerticalLayout>
```

---

## 6. 控件总览与常用属性

> 完整属性见 `Help/Duilib属性列表.html`；示例见 `ControlDemo/skin/main.xml`。

### 6.1 通用属性（几乎所有控件都支持）

| 属性 | 说明 | 示例 |
|---|---|---|
| `name` | 控件名，代码 FindControl 用 | `name="btnOk"` |
| `text` | 显示文本 | `text="确定"` |
| `font` | 字体 id（对应 `<Font>`） | `font="0"` |
| `textcolor` | 文字颜色 | `0xFF333333`（ARGB） |
| `bkcolor` / `bkcolor2` | 背景色（支持渐变） | |
| `bkimage` | 背景图 | `file='a.png' corner='4,4,4,4'` |
| `enabled` / `visible` | 禁用/隐藏 | `enabled="false"` |
| `bordersize` / `bordercolor` | 边框 | |
| `inset` | 内容内边距 | `2,2,2,2` |
| `align` / `valign` | 文字水平/垂直对齐 | `center` / `vcenter` |
| `showhtml` | 支持 `<a>`/`<b>`/`<i>` 内联标签 | `showhtml="true"` |
| `tooltip` | 悬浮提示 | |

**颜色格式**：`0xAARRGGBB`，如 `0xFFFF0000` 不透明白里透红。

### 6.2 基础控件

```xml
<!-- Label 静态文本 -->
<Label text="标题" font="1" textcolor="0xFF2D5BD7" align="center"/>

<!-- Text：与 Label 类似但支持选中复制 -->
<Text text="可选中复制的文本"/>

<!-- Button：五态图片 + 五态颜色 -->
<Button name="btnOk" text="确定" width="120" height="32"
        normalimage="file='btn.png' corner='4,4,4,4'"
        hotimage="..." pushedimage="..." focusedimage="..." disabledimage="..."
        hotbkcolor="0xFF3A7AFF" pushedbkcolor="0xFF2A62D9" hottextcolor="0xFFFFFFFF"/>

<!-- FadeButton：继承 Button，悬浮有淡入淡出过渡 -->
<FadeButton text="渐变按钮" normalimage="..." hotimage="..."/>

<!-- Option 单选/复选：group 相同的互斥 -->
<Option text="男" group="sex" selected="true"/>
<Option text="复选框" width="120"/>          <!-- 无 group 即复选 -->
<CheckBox text="CheckBox 控件"/>

<!-- GroupBox 分组框（继承 HorizontalLayout） -->
<GroupBox text="分组" bordersize="1" bordercolor="0xFFD8DCE3">
  <Option text="选项" group="g1"/>
</GroupBox>

<!-- Progress 进度条 -->
<Progress name="pg" value="60" min="0" max="100" forecolor="0xFF3A7AFF" bkcolor="0xFFE2E8F4"/>

<!-- Slider 滑块（继承 Progress） -->
<Slider name="sl" value="30" thumbsize="14,14"/>   <!-- 通知：DUI_MSGTYPE_VALUECHANGED -->

<!-- ScrollBar 一般通过容器 vscrollbar 属性自动创建，也可独立摆放定制皮肤 -->
```

### 6.3 输入控件

```xml
<!-- Edit：单行输入；tipvalue 空内容时的灰色提示 -->
<Edit name="editUser" tipvalue="请输入用户名" password="false" numberonly="false"
      maxchar="20" nativebkcolor="0xFFFFFFFF" textpadding="6,2,6,2"/>

<!-- RichEdit：多行富文本；vscrollbar/autovscroll 常开 -->
<RichEdit name="rich" vscrollbar="true" multiline="true" wantreturn="true"
          wanttab="false" rich="true" tipvalue="请输入…"/>

<!-- Combo：下拉框，子项为 ListLabelElement -->
<Combo name="city" itemfont="0" itemselectedbkcolor="0xFFEAF1FF" textpadding="6,2,24,2">
  <ListLabelElementUI text="北京" selected="true"/>
  <ListLabelElementUI text="上海"/>
</Combo>

<!-- HotKey 热键输入（原生 HOTKEY 封装） -->
<HotKey name="hk" nativebkcolor="0xFFFFFFFF"/>

<!-- DateTime 日期时间选择（原生 DTP 封装） -->
<DateTime name="dt" nativebkcolor="0xFFFFFFFF"/>

<!-- IPAddress 四段 IP 输入（原生 IP Control 封装） -->
<IPAddress name="ip" nativebkcolor="0xFFFFFFFF"/>
```

### 6.4 列表 List（重点）

```xml
<List name="listDemo" vscrollbar="true"
      header="true"                       <!-- 是否显示表头 -->
      headerbkcolor="0xFFF0F3F8"
      itemfont="0" itemtextcolor="0xFF333333"
      itemselectedbkcolor="0xFFE3EEFF"    <!-- 选中行背景 -->
      itemhotbkcolor="0xFFF0F6FF"         <!-- 悬浮行背景 -->
      itemaltbk="true" altrowcolor="0xFFF7F9FC"   <!-- 隔行变色 -->
      multiselect="false">                <!-- true 支持多选 -->
  <ListHeader height="30">
    <ListHeaderItem text="姓名" width="140" dragable="true" sepwidth="4"/>  <!-- 列宽可拖 -->
    <ListHeaderItem text="部门" width="160"/>
  </ListHeader>
  <!-- 行控件由代码动态添加 -->
</List>
```

三种行类型：

| 行类型 | 用途 |
|---|---|
| `CListLabelElementUI` | 纯文本单列行（简单列表） |
| `CListTextElementUI` | **多列文本行**，配合 ListHeader；`SetText(列, 文本)` |
| `CListContainerElementUI` | **容器行**，行内可放任意控件（按钮/滑块/图片…） |

```cpp
// 多列文本行
CListTextElementUI* pRow = new CListTextElementUI;
pRow->SetOwner(pList);          // 必须先 SetOwner！
pList->Add(pRow);
pRow->SetText(0, _T("张三"));
pRow->SetText(1, _T("研发部"));

// 容器行
CListContainerElementUI* pRow2 = new CListContainerElementUI;
pRow2->SetFixedHeight(34);
pList->Add(pRow2);
pRow2->Add(new CButtonUI);      // 行内放按钮
```

### 6.5 树 TreeView

```xml
<TreeView vscrollbar="true" visiblefolderbtn="true" visiblecheckbtn="false">
  <TreeNode text="根节点" expland="true">
    <TreeNode text="子节点" expland="true">
      <TreeNode text="叶子"/>
    </TreeNode>
  </TreeNode>
</TreeView>
```

### 6.6 菜单 Menu（代码驱动）

```xml
<!-- menu.xml -->
<Menu width="180" bkcolor="0xFFFFFFFF" bordersize="1">
  <MenuElement name="m1" text="打开" height="30" font="0"/>
  <MenuElement name="sep" height="6" linetype="true"/>          <!-- 分隔线 -->
  <MenuElement name="m2" text="自动刷新" checkitem="true" ischeck="false"/>  <!-- 可勾选 -->
  <MenuElement name="m3" text="子菜单">
    <MenuElement text="子项" height="28"/>
  </MenuElement>
</Menu>
```

```cpp
// WM_CONTEXTMENU 或按钮点击时弹出
POINT pt; ::GetCursorPos(&pt);
CMenuWnd::CreateMenu(NULL, _T("skin\\menu.xml"), pt, &m_pm, NULL,
                     eMenuAlignment_Left | eMenuAlignment_Bottom);
// 菜单项点击会以 DUI_MSGTYPE_MENU 通知到宿主 Notify
```

### 6.7 动效与多媒体

```xml
<!-- GIF 动画 -->
<GifAnim bkimage="anim/loading.gif" autoplay="true" autosize="false"/>

<!-- 纯绘加载动画（无资源依赖） -->
<Loading time="120" spoke="8" thickness="4" outradius="18" innerradius="10" color="0xFF3A7AFF"/>

<!-- 滚动公告文字（代码 BeginRoll 启动） -->
<RollText text="滚动公告…" font="0"/>

<!-- 网页：IE 内核；编译期开启 DUILIB_USE_WEBVIEW2 且装有 Runtime 时自动用新内核 -->
<WebBrowser name="web" homepage="https://example.com" autonavi="true"/>

<!-- ActiveX：clsid 实例化任意 COM 控件；Flash 即其特化 -->
<ActiveX clsid="{8856F961-340A-11D0-A96B-00C04FD705A2}"/>
```

---

## 7. 消息处理与事件响应

### 7.1 消息流转全景

```
Win32 消息 → 窗口 HandleMessage()
              ├─ 框架消息（WM_CREATE/WM_CLOSE/WM_NCHITTEST…）自己处理
              ├─ CPaintManagerUI::MessageHandler()     ← 控件消息总入口
              │     ├─ PreMessageHandler（消息过滤器，IMessageFilterUI）
              │     ├─ 按坐标 FindControl 找命中控件
              │     ├─ 生成 TEventUI → 控件 Event()（产生 UIEVENT_CLICK 等）
              │     └─ SendNotify → INotifyUI::Notify() ← 你的业务入口
              └─ 未处理 → DefWindowProc（千万别吞！否则光标异常等怪病）
```

### 7.2 Notify 常用消息类型

```cpp
void Notify(TNotifyUI& msg)
{
    if (msg.sType == DUI_MSGTYPE_CLICK)          {}  // 按钮/菜单点击
    if (msg.sType == DUI_MSGTYPE_SELECTCHANGED)  {}  // Option 选中变化
    if (msg.sType == DUI_MSGTYPE_VALUECHANGED)   {}  // Slider/Edit 值变化
    if (msg.sType == DUI_MSGTYPE_ITEMSELECT)     {}  // List/Combo 选中项变化
    if (msg.sType == DUI_MSGTYPE_ITEMACTIVATE)   {}  // List 双击
    if (msg.sType == DUI_MSGTYPE_MENU)           {}  // 菜单项点击
    if (msg.sType == DUI_MSGTYPE_LINK)           {}  // showhtml 链接
    if (msg.sType == DUI_MSGTYPE_TIMER)          {}  // SetTimer 通知
    if (msg.sType == DUI_MSGTYPE_WINDOWINIT)     {}  // 窗口初始化完成

    // msg.pSender：谁发的
    // msg.wParam/msg.lParam：附带的索引等
    // msg.ptMouse：鼠标坐标
}
```

### 7.3 定时器

```cpp
// 库内定时器（随控件生命周期管理，推荐）
m_pm.SetTimer(pControl, 1001, 60);      // 控件会收到 DUI_MSGTYPE_TIMER
m_pm.KillTimer(pControl, 1001);

// 控件内部使用
void MyControl::DoEvent(TEventUI& event) {
    if (event.Type == UIEVENT_TIMER && event.wParam == 1001) { ... }
}
```

---

## 8. 无边框窗口、圆角与自绘标题栏

这是桌面软件的标准需求，核心四步（示例完整实现见 `ControlDemo.cpp`）：

**① 创建时去边框**

```cpp
pWin->Create(NULL, _T("标题"),
    (WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU)) | WS_VISIBLE,
    WS_EX_WINDOWEDGE);
```

**② XML 配置拖拽与缩放**

```xml
<Window size="1180,760" sizebox="4,4,6,6" caption="0,0,0,32" roundcorner="8,8">
```

- `sizebox`：边缘 4px 内返回 HTTOPLEFT 等实现缩放
- `caption`：顶部 32px 内、**命中的是静态控件**时返回 HTCAPTION，系统接管拖动
- 按钮等交互控件在 caption 内则正常点击（`IsInStaticControl` 判断）

**③ WM_NCHITTEST 命中测试**（逻辑抄自 `WindowImplBase::OnNcHitTest`）

```cpp
case WM_NCHITTEST: {
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ::ScreenToClient(m_hWnd, &pt);
    RECT rcSize = m_pm.GetSizeBox(), rcCap = m_pm.GetCaptionRect();
    // ...四角/四边返回 HT*；caption 内静态控件返回 HTCAPTION...
    return HTCLIENT;
}
```

**④ WM_SIZE 里建圆角 RGN**

```cpp
case WM_SIZE: {
    SIZE sz = m_pm.GetRoundCorner();     // 来自 XML roundcorner
    RECT rc; ::GetWindowRect(m_hWnd, &rc);
    ::OffsetRect(&rc, -rc.left, -rc.top);
    HRGN hRgn = ::CreateRoundRectRgn(0, 0, rc.right+1, rc.bottom+1, sz.cx, sz.cy);
    ::SetWindowRgn(m_hWnd, hRgn, TRUE);
    ::DeleteObject(hRgn);
    break;
}
```

**常见坑**：
- `WM_NCACTIVATE` 直接 `return TRUE`，否则无边框窗口顶部闪白线
- 消息链**最后必须兜底 DefWindowProc**，吞掉 WM_SETCURSOR 会导致鼠标变沙漏
- 圆角窗口若要阴影，配合 `CShadowUI`（管理器自带，`m_pm.GetShadow()->ShowShadow(true)`）
- **若窗口过程在 `OnFinalMessage` 中 `delete this`**：`WM_NCDESTROY` 回调返回后绝不能再写对象成员；
  库的 `__WndProc`/`__ControlProc` 已按此模式修复（先置 `m_hWnd=NULL`、回调后不写 `pThis`）

---

## 9. 资源管理：路径、zip 包与换肤

```cpp
// 方式一：文件夹（开发期方便）
CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath() + _T("skin"));
// 此时 XML 中 bkimage="anim/a.gif" 实际读取 skin\anim\a.gif

// 方式二：zip 资源包（发布期推荐，可加密）
CPaintManagerUI::SetResourceZip(_T("skin.zip"), true /*缓存*/, _T("可选密码"));
// 加载顺序：zip 内找不到 → 回落磁盘文件（GetResourcePath）
```

- XML 里 `<Image>` 节点可预声明图片属性（mask 镂色/HSL 等）
- `bkimage="file='a.png' source='0,0,20,20' corner='4,4,4,4' hsl='true'"`：
  `corner` 四边不拉伸（九宫格），是按钮皮肤的关键属性
- 换肤 = 换 zip 后调用 `ReloadSkin()`（内部 ReloadImages + 全窗口重绘）

---

## 10. 高 DPI、HSL 调色与阴影

```cpp
// DPI 自适应：设计稿按 96dpi 写死即可，运行时一键缩放
m_pm.SetDPI(120);        // 125%；XML 里的尺寸/字体全部自动放大

// HSL 调色：只对 bkimage 图片生效（做主题色变换神器）
m_pm.SetHSL(true, 180, 100, 100);   // 开启并调色
m_pm.SetHSL(false, 0, 0, 0);        // 关闭并还原

// 窗口阴影
m_pm.GetShadow()->ShowShadow(true);
m_pm.GetShadow()->SetSize(6);            // 阴影扩散半径
m_pm.GetShadow()->SetDarkness(150);      // 浓度
// 图片模式阴影：SetImage("shadow.png") + SetShadowCorner(...)
```

---

## 11. 常见问题与调试技巧

| 症状 | 原因 | 解决 |
|---|---|---|
| **鼠标变沙漏 / 点击无反应** | 消息链吞掉了未处理消息 | `MessageHandler` 返回 false 时必须 `DefWindowProc`，见第 8 节 |
| XML 加载失败界面空白 | 路径错误/编码错误 | XML 必须带 `encoding="UTF-8"` 且存为 UTF-8（建议带 BOM）；打印 GetInstancePath 检查 |
| **窗口完全不显示（白/透明）** | 加了 `WS_EX_LAYERED` 却没调用 `SetLayeredWindowAttributes` | 去掉该样式，或用 `m_pm.SetOpacity(255)` 让管理器接管；圆角用 `SetWindowRgn`/DWM 即可，无需 layered |
| **窗口拖不动** | 拦截了 `WM_SYSCOMMAND` 的 `SC_MOVE` | 不要拦截——HTCAPTION 拖动依赖系统 SC_MOVE 流程 |
| **圆角锯齿** | `SetWindowRgn` 是 1-bit 蒙版 | Win11 用 `DwmSetWindowAttribute(33, DWMWCP_ROUND)` 系统抗锯齿圆角；Win10 回退 RGN（属系统限制） |
| **SetTimer 断言 (uElapse>0)** | 调用方传了 0（如 RollText 滚动时长漏乘 1000） | 库已加固为静默拒绝；调用侧检查 `SetTimer` 第三参必须 >0 |
| **右键菜单弹出即断言崩溃** | `CreateMenu` 传了**绝对路径**——`LoadFromFile` 会拼接 ResourcePath 导致路径无效 → `AttachDialog(NULL)` | 只传**相对文件名**（如 `_T("menu.xml")`），资源由 `SetResourcePath` 统一管理 |
| **AttachDialog(NULL) 断言** | 皮肤 XML 加载失败后未判空就挂载 | 库已加固（`AttachDialog`/`InitControls` 入口判空拒绝）；调用侧检查 `builder.Create` 返回值 |
| **右键菜单反复弹出/关闭后卡死闪退**（已修复） | 多重 use-after-free：① 菜单窗口销毁（delete this）后窗口过程继续用已释放的 this（`m_pm.MessageHandler`）② `RBroadcast` 遍历时销毁 receiver 迭代器失效 ③ `ParentProc` 尾部经已释放 `pThis` 读原过程指针 ④ `CMenuElementUI::EstimateSize` 解引用未设置的 `m_pOwner` | 库已修复：各销毁分支立即 return、快照遍历、`CShadowUI`/观察者链析构清理、EstimateSize 判空 |
| 链接错误 LNK2019/2038 | 运行库或字符集不一致 | Demo 与库的 `/MT`-`/MD`、Unicode/多字节、Win32/x64 必须一致 |
| 控件不显示 | 容器未给高度 / 控件 float 未定位 | 检查父容器 `height` 与自身 `width/height` |
| 列表行不显示 | 忘记 `SetOwner` | `CListTextElementUI` 先 `SetOwner(pList)` 再 `Add` |
| 中文字符串乱码 | 源文件编码 | 源码保存为 **UTF-8 with BOM** |
| 崩溃在 FindControl | root 未 AttachDialog / name 拼错 | 判空 + 统一在 XML 顶部用注释管理 name |
| 图片不显示 | 资源路径/zip 内路径 | `bkimage` 路径相对 ResourcePath；zip 内用 `/` 分隔 |
| 刷新异常/残影 | 复杂布局未开双缓冲 | 窗口 `bktrans="true"` 或控件 `bordersize` 排查；调试期开 `showdirty="true"` |

**调试技巧**：
1. `Window showdirty="true"` 红框显示每个重绘区域，定位布局问题
2. `SetRoundCorner(0,0)` 排查圆角 RGN 引发的裁剪问题
3. DUITRACE 宏输出 Debug 日志（Debug 版生效）
4. `m_pm.FindControl(name)` 返回值**必须判空**——XML 名字写错不报错只返回 NULL

---

## 12. 工程实践建议

1. **皮肤模块化**：公共控件样式放 `<Default>`/`<Style>`；重复 UI 用 `<Include>`/`ChildLayout` 拆文件
2. **name 规范**：控件名统一前缀（`btn*/lbl*/edt*/lst*`），在 XML 顶部注释登记
3. **业务与界面解耦**：Notify 里只做转发，业务写在独立类；控件查找封装成 `GetCtrl(name)` 带断言
4. **资源打包**：发布用 zip（`SetResourceZip` 缓存模式），图片优先 PNG，九宫格 `corner` 减少切图
5. **升级注意**：本仓库已修复大量上游 bug（消息路径判空、COM 引用计数、属性解析越界等），
   修改库源码前先看 `BUGFIX_PLAN.md` 避免回归
6. **多窗口**：每个窗口一个 `CPaintManagerUI`；模态用 `ShowModal()`，非模态用 `ShowModalFake()` + `Close()`

---

## 13. 实战经验：崩溃案例分析

> 本章来自对本仓库的真实修复（完整清单见 `BUGFIX_PLAN.md`）。这些坑在官方 duilib 及各衍生版中普遍存在，
> 无论你用的是哪个分支都值得通读。每条都是**实际导致崩溃/卡死/闪退**的问题，而非理论风险。

### 13.1 右键菜单生命周期——最隐蔽的一组 use-after-free

右键菜单（`CMenuWnd`）是 DuiLib 中**对象生命周期最复杂的组件**：一个 C++ 对象同时是
窗口过程宿主、观察者接收者、控件树管理者。官方代码在这条链上有 5 处 use-after-free，
组合出“右键菜单间歇性卡死/闪退”（实测 1~3 次操作必现）。

**根因 ①：销毁后继续使用 this（最致命）**

`Close()` 改为同步 `DestroyWindow` 后，`WM_CLOSE` 消息处理中会同步触发
`WM_NCDESTROY → OnFinalMessage → delete this`。此时窗口过程 `HandleMessage` 的栈帧还在，
返回后**继续执行 `m_pm.MessageHandler(...)`——而 this 已被 delete**：

```cpp
// ❌ 错误模式（官方原始代码）：
case WM_CLOSE:
    if( m_pOwner != NULL ) { ... }
    ::DestroyWindow(m_hWnd);      // 同步销毁 → OnFinalMessage → delete this
    break;                        // ← break 后继续往下走！
...
if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) )  // this 已释放 → 崩溃
    return lRes;
```

```cpp
// ✅ 正确模式（已修复）：
case WM_CLOSE:
    if( m_pOwner != NULL ) { ... }
    ::DestroyWindow(m_hWnd);
    return 0;                     // ← 销毁后必须立即 return，绝不走后续代码
case WM_KILLFOCUS:
    lRes = OnKillFocus(...);      // OnKillFocus 内 RBroadcast 也会销毁 this
    if (isClosing || !::IsWindow(m_hWnd)) return 0;   // 同理
    break;
case WM_KEYDOWN:
    if (wParam == VK_ESCAPE) { Close(); return 0; }   // 同理
```

> **通用规则**：窗口过程中任何可能触发 `DestroyWindow`/`delete this` 的调用之后，
> 必须立即 `return`，不得再访问任何成员或虚函数。

**根因 ②：`__WndProc` 在 `OnFinalMessage` 之后写对象成员**

DuiLib 的窗口销毁回调约定是 `OnFinalMessage` 里 `delete this`（示例代码也这么写）。
但官方 `__WndProc` 的 `WM_NCDESTROY` 分支是：

```cpp
// ❌ 官方原始代码：
pThis->OnFinalMessage(hWnd);      // 内部可能 delete this
pThis->m_hWnd = NULL;             // ← 写已释放内存！堆破坏
return lRes;
```

```cpp
// ✅ 已修复：先置空再回调，回调后不写
pThis->m_hWnd = NULL;
pThis->OnFinalMessage(hWnd);
return lRes;
```

堆破坏的可怕之处在于**崩溃点远离破坏点**——你可能销毁了菜单，却在几秒后点别处时崩溃，
调试器栈完全对不上。本项目实测崩溃偏移 `NULL+0x13f3c` 正是这种“幽灵崩溃”。

**根因 ③：观察者广播时销毁 receiver → 迭代器失效**

菜单关闭靠观察者广播（`RBroadcast`），而 receiver 的 `Receive` 内部会 `Close → delete this
→ RemoveObserver → observer 的 vector erase`：

```cpp
// ❌ 官方原始代码：直接遍历，erase 后迭代器失效
for (auto it = pReceivers_->rbegin(); it != pReceivers_->rend(); ++it)
    (*it)->Receive(param);
```

```cpp
// ✅ 已修复：先拷贝快照再遍历
ReceiversVector snapshot(*pReceivers_);
for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it)
    (*it)->Receive(param);
```

**根因 ④：`CMenuElementUI::EstimateSize` 解引用未初始化的 `m_pOwner`**

`CreateMenu(NULL, ...)` 独立菜单模式下，XML 解析出的 `MenuElement` 的 `m_pOwner` 从未被设置
（只有 owner 模式会设置），而 `EstimateSize` 直接 `m_pOwner->GetListInfo()`：

```cpp
// ✅ 已修复（判空早退）：
SIZE CMenuElementUI::EstimateSize(SIZE szAvailable)
{
    if (GetManager() == NULL) return CDuiSize(m_cxyFixed);
    ...
    if (cXY.cy == 0) {
        if (m_pOwner == NULL) return szRetFix;   // XML 模式用固定行高
        ...
    }
}
```

**根因 ⑤：`CShadowUI` 全局 map 残留悬空条目**

阴影窗口通过全局 `std::map<HWND, CShadowUI*>` 与宿主关联，但官方析构函数不摘除自己。
菜单窗口（含阴影）销毁后 map 里留下悬空指针，下次任何消息触发 `ParentProc` 查表即崩：

```cpp
// ✅ 已修复：析构按指针值摘除（不依赖 m_pManager 状态）
CShadowUI::~CShadowUI(void)
{
    auto& shadowMap = GetShadowMap();
    for (auto it = shadowMap.begin(); it != shadowMap.end(); ) {
        if (it->second == this) it = shadowMap.erase(it);
        else ++it;
    }
}
```

同时 `ParentProc` 的 `WM_DESTROY`/`WM_NCDESTROY` 分支用**局部变量**保存原窗口过程——
尾部 `CallWindowProc(pThis->m_OriParentProc, ...)` 在 pThis 释放后是悬空读。

**修复后的验证方法**：Debug 版 40 轮 × （右键弹菜单→点击外部关闭）无崩溃，
且库内生命周期日志每轮完整闭合（`Close-enter → Final-enter → RmObs-done`）。

### 13.2 皮肤路径与 `AttachDialog(NULL)`

`CreateMenu` **必须传相对文件名**。因为 `CMarkup::LoadFromFile` 内部做
`ResourcePath + 传入路径` 拼接：

```cpp
// ❌ 传绝对路径：
CMenuWnd::CreateMenu(NULL, _T("D:\\app\\skin\\menu.xml"), ...);
// 拼接结果 "D:\app\skin\D:\app\skin\menu.xml" → 打不开 → builder.Create 返回 NULL
// → AttachDialog(NULL) → InitControls 断言崩溃

// ✅ 传相对文件名：
CMenuWnd::CreateMenu(NULL, _T("menu.xml"), ...);
```

库侧已加固：`AttachDialog`/`InitControls` 对 NULL 静默拒绝（不再断言中断）。
但**调用侧永远要检查 `builder.Create` 的返回值**——返回 NULL 说明 XML 没加载成功，
静默继续只会让崩溃延后到更难查的地方。

### 13.3 `SetTimer` 传 0 → 断言与状态破坏

`RollText` 的 `BeginRoll(dir, span, maxTime)` 第三参是**秒**，内部会 `×1000` 传给 `SetTimer`。
传 0 → `SetTimer(uElapse=0)` → Debug 断言；点“忽略”后定时器管理表进入异常状态 →
后续随机卡死。

```cpp
// ❌ pRoll->BeginRoll(ROLLTEXT_LEFT, 60, 0);     // 第3参 0 → SetTimer(0)
// ✅ pRoll->BeginRoll(ROLLTEXT_LEFT, 60, 60);    // 给足滚动总时长（秒）
```

库侧已加固：`SetTimer` 对 `uElapse==0` 静默拒绝。**教训**：封装带单位换算的 API 时，
在库层把非法参数挡住（return false），不要只依赖断言。

### 13.4 `WS_EX_LAYERED` 陷阱：窗口“显示成功”却全透明

给窗口加 `WS_EX_LAYERED` 但**不调用** `SetLayeredWindowAttributes` 或 `UpdateLayeredWindow`，
窗口会**完全不可见**——`FindWindow`/`GetWindowRect` 一切正常，进程“运行正常”，
用户却看到“程序没界面”。本项目实测踩中。

```cpp
// ❌ 圆角不需要 layered，不要顺手加：
pFrame->Create(..., WS_EX_LAYERED | WS_EX_WINDOWEDGE);   // 窗口隐身！
// ✅ 圆角用 SetWindowRgn / DWM 实现（见第 8 章），无需 layered
```

需要半透明时用管理器接口：`m_pm.SetOpacity(255)`（内部会正确调用 layered API）。

### 13.5 `ParentProc`/子类化的通用铁律

`CShadowUI` 会把宿主窗口子类化（替换 `GWLP_WNDPROC` 为静态 `ParentProc`），
通过全局 map 从 HWND 反查 this。这套机制有三条铁律：

1. **销毁时先反子类化再摘 map**——顺序错了后续消息会进悬空的过程或查到悬空的 this
2. **map 查找失败时走 `DefWindowProc`**，绝不能解引用查不到的 NULL（官方代码 `_ASSERT` 后直接用）
3. **对象析构时按指针值从 map 摘除自己**，不能依赖“销毁消息一定会来”

### 13.6 排查这类问题的实战方法论

1. **WER 事件日志**：`事件查看器 → 应用程序 → ID 1000`，记录崩溃模块 + RVA 偏移。
   偏移小（如 `NULL+0x13f3c`）≈ 空指针+成员偏移；偏移飘忽 = 悬空指针/堆破坏。
2. **MAP 文件定位**：链接选项加 `MapFileName` + `MapExports`，
   用崩溃 RVA 在 map 中二分找所属函数。注意 **`/GL` 全程序优化下符号归属不可靠**
   （内联函数都归到邻近符号名下），此时改用 Debug 版复现。
3. **库内文件日志**：断言/崩溃路径上打 `FILE* fp = _tfopen(...)` 式日志
   （比 OutputDebugString 不依赖工具），复现后看**最后一条日志写到哪**，
   即缩小到两个打点之间的调用。
4. **Debug/Release 行为差异**：`/GL`+`MinSpace` 的 SRelease 与 Debug 的崩溃概率、
   崩溃点都不同（堆布局/优化内联差异）。Debug 复现不了不代表 Release 没有——
   两边都要压测。
5. **二分法注释**：怀疑某段初始化代码时，注释掉它压测对比（本项目靠这招排除了
   `FillContainerList` 的误判），但注意**必须再恢复并二次确认**，避免误诊。

---

## 14. HTML/CSS 界面渲染（HtmlBox 控件）

> 本章是 DuiLib-X 的核心增强：**用 HTML/CSS 写界面，不依赖任何浏览器内核**。
> 排版交给 [litehtml](https://github.com/litehtml/litehtml)（BSD-3，约 2 万行 C++），
> 绘制与交互全部由 DuiLib 原生完成。

### 14.1 原理：三层结构

```
你的 HTML 字符串
      ↓
litehtml 解析（内置 gumbo HTML5 解析器）+ CSS 级联 + 盒模型排版
      ↓ 算出每个元素的 位置/尺寸/颜色/字体
CDuiHtmlContainer（document_container 回调实现）
      ↓ 把排版结果用 GDI 画到 DuiLib 的内存 DC
CHtmlBoxUI（DuiLib 控件，可直接放进 XML 或程序化创建）
```

litehtml 是**排版计算引擎，不是绘制引擎**——它只负责“算出每个盒子在哪、多大、什么颜色”，
真正画像素要靠宿主实现 `document_container` 的 24 个虚函数。
DuiLib-X 把这 24 个回调全部实现在 `DuiLib/Control/UIDuiHtmlContainer.cpp`。

### 14.2 最小示例

```cpp
#include "UIlib.h"
using namespace DuiLib;

CHtmlBoxUI* pBox = new CHtmlBoxUI();
pBox->SetName(_T("htmlBox"));
pBox->SetBkColor(0xFFEEF2F7);          // 控件底色（必须设，否则露窗口黑底）
pBox->SetManager(m_pm, NULL, true);
m_pm->AttachDialog(pBox);              // 作为根控件
pBox->SetHtml(PAGE_MAIN);              // 或 SetHtmlFromFile(_T("page.html"))
```

XML 写法：

```xml
<HtmlBox name="htmlBox" bkcolor="0xFFEEF2F7" html="page.html" />
<!-- 或直接内联：htmltext="&lt;div&gt;你好&lt;/div&gt;" -->
```

### 14.3 表单控件：litehtml 不提供，DuiLib-X 自己实现

litehtml 的 master CSS 里对 `input/select/button/textarea` 只给了
`display: inline-block`，**不会画任何东西**（它设计上就把表单交给宿主）。

DuiLib-X 通过 `create_element` 回调把这些标签接管为 `CHtmlFormElement`
（`DuiLib/Control/UIDuiHtmlForm.h/.cpp`），按“替换元素(replaced element)”处理：

| 标签 | 行为 |
|---|---|
| `<input type=text>` | 白底框 + 真实键盘输入 + 光标移动/退格/删除 |
| `<input type=password>` | 同上，内容显示为 `*` |
| `<input type=checkbox>` | 方框 + 勾选标记，点击切换 |
| `<input type=radio>` | 圆点，点击选中 |
| `<input type=hidden>` | 不绘制，但 `GetValue` 可读到 |
| `<select>` | 下拉框（显示当前项 + 三角箭头） |
| `<button>` | 按钮，点击触发 `DUI_MSGTYPE_HTMLFORMSUBMIT` |
| `<textarea>` | 多行文本框 |

**键盘输入是怎么接上的**：`CHtmlBoxUI::DoEvent` 收到 `UIEVENT_CHAR` / `UIEVENT_KEYDOWN`
就转发给容器，容器再转给当前焦点控件（`m_pFocusedForm`）。
焦点由鼠标点击时的 `HitTestForm` 决定。

### 14.4 数据双向交互

```cpp
// 读
CDuiString user = pBox->GetValue(_T("user"));        // input/select/textarea 的值
bool remember   = pBox->IsChecked(_T("remember"));   // checkbox/radio 是否选中

// 写
pBox->SetValue(_T("user"), _T("李四"));
pBox->SetChecked(_T("remember"), false);
```

三个通知消息：

| 消息 | 触发 | `wParam` |
|---|---|---|
| `DUI_MSGTYPE_HTMLLINK` | `<a href>` 被点击 | 链接地址 |
| `DUI_MSGTYPE_HTMLFORMSUBMIT` | `<button>` 被点击 | 控件 `name` |
| `DUI_MSGTYPE_HTMLCHANGED` | 任意字段值变化 | 控件 `name` |

```cpp
void Notify(TNotifyUI& msg) override
{
    if (msg.sType == DUI_MSGTYPE_HTMLFORMSUBMIT) {
        CHtmlBoxUI* pBox = static_cast<CHtmlBoxUI*>(m_pm->FindControl(_T("htmlBox")));
        if (pBox == NULL) return;
        CDuiString s;
        s.Format(_T("用户名=%s 城市=%s 记住=%s"),
                 pBox->GetValue(_T("user")).GetData(),
                 pBox->GetValue(_T("city")).GetData(),
                 pBox->IsChecked(_T("remember")) ? _T("是") : _T("否"));
        ::MessageBox(m_hWnd, s.GetData(), _T("后端收到"), MB_OK);
    }
}
```

### 14.5 支持范围

| 类别 | 支持 |
|---|---|
| 文本 | h1-h6 / p / b / i / u / font / br / span，颜色字号字重装饰 |
| 布局 | div 嵌套、margin / padding / border、`display:inline-block`、table、hr |
| CSS | 标签/类/ID/属性/后代选择器、级联、继承、`<style>` 块、外部 css、`@import`、`:hover`/`:active`/`:link` |
| 表单 | 见 14.3 |
| 图片 | 本地图片（含 DuiLib zip 资源包），`get_image_size` / `load_image` 走 `CRenderEngine` |
| 不支持 | JavaScript、CSS 动画/过渡、canvas / video / iframe、Web Font |

### 14.6 常见坑（都是本仓库实际踩过的）

**坑 ①：白屏——`Draw` 传给 litehtml 的 clip 用了窗口坐标**

`litehtml::document::draw(hdc, x, y, clip)` 的 `clip` 参数是**文档坐标系**的矩形，
litehtml 拿它和元素在文档坐标下的位置做相交判断。若传了含控件偏移的窗口坐标矩形，
相交结果为空 → 一个元素都不画 → 纯白。

```cpp
// ❌ 错：传控件绝对坐标
litehtml::position clip(rcItem.left, rcItem.top, w, h);

// ✅ 对：传文档坐标原点
litehtml::position clip(0, 0, w, h);
```

**坑 ②：白屏——`Render` 前没告诉引擎客户区尺寸**

`document::render(max_width)` 内部会调 `get_client_rect()` 拿高度。
若此时 `m_rcClient` 还是全 0，排版按 0 宽度算，所有盒子塌陷成 0。

```cpp
// ✅ 必须在 render 前设置
m_rcClient.right  = m_rcClient.left + maxWidth;
m_rcClient.bottom = m_rcClient.top + 600;   // 初始高度，内容会撑开
m_doc->render(maxWidth);
```

**坑 ③：滚动时内容错位——没对 DC 做 y 偏移**

litehtml 的绘制坐标始终从文档 (0,0) 开始，滚动要由宿主做坐标变换：

```cpp
POINT ptOld = { 0, 0 };
::GetViewportOrgEx(hdc, &ptOld);
::SetViewportOrgEx(hdc, ptOld.x, ptOld.y - scrollY, NULL);
m_doc->draw((litehtml::uint_ptr)hdc, 0, 0, &clip);
::SetViewportOrgEx(hdc, ptOld.x, ptOld.y, NULL);
```

**坑 ④：`set_clip` / `del_clip` 空实现**

litehtml 对 `overflow: hidden` 的元素会调 `set_clip`。若你只是建了个 Region 存起来、
**从未 `SelectClipRgn` 到 DC**，裁剪完全不生效，滚动时子内容会画到外面。
正确做法是容器在 `Draw` 期间保存 `m_hDrawDC`，`set_clip` 里真正应用到它。

**坑 ⑤：控件没设背景色 → 露窗口黑底**

`CHtmlBoxUI` 继承自 `CVerticalLayoutUI`，默认透明。窗口底色是黑的，
所以一开始表现为“一片黑”。`DoPaint` 里先填白底 + 外部 `SetBkColor` 一起解决。

**坑 ⑥：链接库没同步，改了源码却“没生效”**

本仓库 `DuiLib.vcxproj` 的静态库输出在 `DuiLib/Lib/DuiLib.lib`，
而 Demo 链接的是 `Lib/SRelease_Win32.lib`。**必须手工拷贝**，
否则 msbuild 认为 exe 已是最新、跳过链接，你会一直看到旧画面：

```bat
copy /Y DuiLib\Lib\DuiLib.lib Lib\SRelease_Win32.lib
```

**坑 ⑦：源码文件编码**

工程用 UTF-8（**带 BOM**）保存。若新建的 `.cpp/.h` 是 UTF-8 **无 BOM**，
MSVC 会按 GBK 读取，中文字符的尾字节会吞掉后面的引号/换行，报出
`C3688 文本后缀无效`、`C2143 语法错误` 这类看不懂的错误。加 BOM 即可。

### 14.7 自测

`HtmlDemo/HtmlFormTest.cpp` 是一个**无窗口控制台自测**，验证表单链路
（查找、读写、勾选、中英文输入、退格按字符而非字节）：

```bat
msbuild HtmlDemo\HtmlFormTest.vcxproj /p:Configuration=SRelease /p:Platform=Win32
HtmlDemo\bin\HtmlFormTest.exe
```

当前 25 项断言全部通过。

---

| 文件 | 内容 |
|---|---|
| `ControlDemo/ControlDemo.cpp` | 主程序：无边框+圆角窗口、消息路由、全部控件的事件演示 |
| `ControlDemo/skin/main.xml` | 7 个 Tab 页覆盖属性列表全部控件 |
| `ControlDemo/skin/menu.xml` | 右键菜单示例 |
| `ControlDemo/skin/child_panel.xml` | ChildLayout 嵌入示例 |
| `ControlDemo/ControlDemo.vcxproj` | SRelease(静态)/Release(动态) × Win32/x64 四种配置 |
| `HtmlDemo/HtmlDemo.cpp` | 纯 HTML 界面演示（登录表单 + 事件回调） |
| `HtmlDemo/HtmlFormTest.cpp` | 表单链路无窗口自测（25 项断言） |
| `DuiLib/Control/UIDuiHtmlContainer.*` | litehtml `document_container` 全部 24 个回调实现 |
| `DuiLib/Control/UIDuiHtmlForm.*` | 表单控件元素（input/select/button/textarea） |
| `DuiLib/Control/UIHtmlBox.*` | HtmlBox 控件对外 API |

编译运行：

```bat
msbuild DuiLib\DuiLib.vcxproj /p:Configuration=SRelease /p:Platform=Win32 ^
        /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.19041.0 ^
        /p:OutDir=..\Lib\ /p:TargetName=SRelease_Win32

msbuild ControlDemo\ControlDemo.vcxproj /p:Configuration=SRelease /p:Platform=Win32

:: 产物：ControlDemo\bin\demo\ControlDemoS.exe（skin 目录需在 exe 旁）
```

## 附录 B：XML 属性书写规范速记

```
坐标/尺寸:  "x,y" 或 "l,t,r,b"（逗号分隔，字段不足自动补 0，容错安全）
颜色:       0xAARRGGBB
BOOL:       true / false
图片:       file='a.png' 或纯路径；可带 corner/source/mask/fade/hsl 等
文字转义:   XML 内引号写作 &quot; 或换用单引号
```
