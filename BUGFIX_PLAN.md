# DuiLib-X 修复记录

完整修复清单，修改库源码前请先阅读避免回归。

## 批次1：严重（崩溃/堆破坏）
- DrawHtmlText 链接数组越界写、MakeShadow 锚点未初始化、GdiplusLoadImage UAF
- AddFontArray、AddImage HSL、WndProc lpCreateParams、CTxtWinHost、ActiveX、List 判空
- 消息路径 m_pRoot 判空（11处）、WebBrowser COM 引用计数、观察者快照遍历

## 批次2：中等（泄漏/UB）
- 属性解析越界读（50+处 DuiParseIntList）、GetMessage 死循环、各类泄漏
- UIList 多选索引、UIMarkup、UIDlgBuilder、UIShadow

## 批次3：低/优化
- 定时器ID冲突、SetResourceZip、Subclass UserData、LoadPlugin 等

## 批次4：遗留项
- 拖拽路径、RichEdit 模块缓存、HACCEL 缓存、GifAnim、ShowModalFake 兜底等

## 批次5：右键菜单生命周期（use-after-free 全链）
- CMenuElementUI::EstimateSize 空指针、RBroadcast 快照遍历
- CMenuWnd 同步销毁 + WM_KILLFOCUS/WM_CLOSE/WM_KEYDOWN 分支立即 return
- __WndProc OnFinalMessage 后不写 pThis、CShadowUI 析构摘除 map
- ParentProc 局部变量保存原过程指针
- 实测：Debug 40 轮、SRelease 10+ 轮 × 15 次菜单循环 PASS

## 批次6：HTML 渲染白屏（litehtml 绘制链）
- **根因①** `Draw()` 传给 `document::draw()` 的 clip 用了窗口绝对坐标，
  litehtml 拿它和文档坐标元素相交 → 结果为空 → 一个元素都不画（纯白）
  修复：clip 改用文档坐标原点 `position(0, 0, w, h)`
- **根因②** `Render()` 前未设置 `m_rcClient`，`get_client_rect()` 返回全 0，
  排版按 0 宽度计算 → 所有盒子塌陷
  修复：render 前设置 `m_rcClient.right = left + maxWidth`
- **根因③** `set_clip`/`del_clip` 空实现（建了 Region 从未 `SelectClipRgn` 到 DC），
  `overflow:hidden` 完全失效
  修复：Draw 期间保存 `m_hDrawDC`，`set_clip` 真正应用到 DC
- **根因④** `Draw()` 无滚动 y 偏移，滚动后内容错位
  修复：`GetViewportOrgEx`/`SetViewportOrgEx` 做变换并恢复
- **根因⑤** `CHtmlBoxUI` 无背景色，DuiLib 默认透明 → 露出窗口黑底（此前的"一片黑"）
  修复：`DoPaint` 白底填充 + 外部 `SetBkColor`
- **根因⑥（最隐蔽）** `DuiLib/Lib/DuiLib.lib` 与 Demo 链接的 `Lib/SRelease_Win32.lib`
  是两个文件，未同步 → msbuild 判定 exe 最新跳过链接 → 改了源码"毫无效果"
  修复：每次编译后 `copy /Y DuiLib\Lib\DuiLib.lib Lib\SRelease_Win32.lib`
- 实测：PrintWindow 抓图确认标题栏 `#2D5BD7`、卡片白底、边框 `#D9E0EA`、文字 `#333` 全部渲染

## 新增功能：HTML 表单控件（litehtml 自身不提供）
- litehtml 的 master CSS 对 input/select/button/textarea 只给 `display:inline-block`，
  不绘制任何内容（设计上交由宿主实现）
- 新增 `CHtmlFormElement`（`Control/UIDuiHtmlForm.h/.cpp`），经 `create_element`
  接管这四类标签，按"替换元素"处理：
  - `get_content_size` 给出尺寸，`draw` 自绘，`is_replaced` 返回 true
  - input(text/password/checkbox/radio/hidden/submit)、select、button、textarea 全覆盖
  - 键盘输入（`InsertChar`/`Backspace`/`DeleteChar`/光标移动），中文按字符而非字节
  - 按钮文字取 `get_text()` 后清空子节点，避免 litehtml 重复绘制成黑块
- `CDuiHtmlContainer` 新增 `FindFormElement` / `HitTestForm` / `InputChar` /
  `InputKeyDown` / `GetRealFont`，并转发 `HTMLFORMSUBMIT` / `HTMLCHANGED` 消息
- `CHtmlBoxUI::GetValue/SetValue/IsChecked/SetChecked` 改为走文档树查控件
- 自测 `HtmlDemo/HtmlFormTest.cpp`：25 项断言全部通过（含中英文输入、退格）
- litehtml Release 配置 RuntimeLibrary 由 MD 改 MT，与 DuiLib Release 对齐

## 新增功能：HTML 界面（HtmlBox）
- 集成 litehtml v0.9（BSD-3）实现零内核 HTML/CSS 渲染
- CHtmlBoxUI 控件：SetHtml/SetHtmlFromFile/GetValue/SetValue/IsChecked/SetChecked
- CDuiHtmlContainer 绘制回调（字体/文本/背景/边框/图片/裁剪/DPI）
- 表单控件映射原生控件、链接点击事件转发