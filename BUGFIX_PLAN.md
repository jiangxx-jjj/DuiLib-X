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

## 批次6：HTML 渲染白屏（早期 litehtml 集成，已弃用该方案）

> 本节记录集成 litehtml 阶段踩的坑。**最终方案已改为完全自研引擎**，
> litehtml 相关代码（`UIDuiHtmlContainer` / `UIDuiHtmlForm`）已删除。

- **根因①** `Draw()` 传给 `document::draw()` 的 clip 用了窗口绝对坐标，
  litehtml 拿它和文档坐标元素相交 → 结果为空 → 一个元素都不画（纯白）
- **根因②** `Render()` 前未设置 `m_rcClient`，`get_client_rect()` 返回全 0，
  排版按 0 宽度计算 → 所有盒子塌陷
- **根因③** `set_clip`/`del_clip` 空实现（建了 Region 从未 `SelectClipRgn` 到 DC），
  `overflow:hidden` 完全失效
- **根因④** `Draw()` 无滚动 y 偏移，滚动后内容错位
- **根因⑤** `CHtmlBoxUI` 无背景色，DuiLib 默认透明 → 露出窗口黑底（此前的"一片黑"）
- **根因⑥（最隐蔽）** `DuiLib/Lib/DuiLib.lib` 与 Demo 链接的 `Lib/SRelease_Win32.lib`
  是两个文件，未同步 → msbuild 判定 exe 最新跳过链接 → 改了源码"毫无效果"

## 批次7：改为完全自研 HTML/CSS 引擎

**弃用 litehtml 的原因**：
- 表单控件（input/select/button/textarea）litehtml 完全不实现，只给 `display:inline-block`
- 不支持 `box-shadow` / `linear-gradient` / `opacity` / `filter` 等现代 CSS3
- `border-radius` 能解析但绘制回调拿不到，需自己反推
- 排版行为不可控，出问题只能读 2 万行第三方源码

**自研引擎架构**（`DuiLib/Control/HtmlEngine/`，约 6000 行）：

| 文件 | 职责 |
|---|---|
| `HtmlTypes.h/.cpp` | 基础类型：Color / Length / 枚举 / Shadow / Gradient / BorderSide / BorderRadius |
| `HtmlDom.h/.cpp` | Node 树 + ComputedStyle（全部计算后样式）+ 标签分类 |
| `HtmlParser.h/.cpp` | HTML5 容错解析（自动闭合、实体解码、style/script 特殊处理） |
| `HtmlCss.h/.cpp` | CSS3 解析 + 选择器匹配（标签/类/ID/属性/后代/子/兄弟/伪类）+ 级联 + UA 默认样式表 |
| `HtmlLayout.h/.cpp` | Block / Inline（自动换行 + text-align）/ Flex / Table / 绝对定位 / 浮动 / 列表项 |
| `HtmlPainter.h/.cpp` | GDI+ 绘制：圆角路径、线性/径向渐变、盒阴影（DIB+AlphaBlend）、文字阴影、表单控件 |
| `HtmlDocument.h/.cpp` | 门面：LoadFromString/File → Relayout → Paint → HitTest/OnClick/InputChar |

**CSS3 实现要点**：
- `border-radius`：四角独立 + 水平/垂直双半径，用 GDI+ `GraphicsPath` 贝塞尔近似圆角，
  背景/边框/子内容裁剪全部按圆角路径处理
- `linear-gradient` / `radial-gradient`：解析角度（deg/rad/turn/to direction）+ 色标（含自动位置分配），
  用 `LinearGradientBrush` / `PathGradientBrush` 绘制
- `box-shadow`：32 位 DIB 逐像素算 alpha（含圆角边缘距离衰减），`AlphaBlend` 贴回，实现真正的半透明模糊
- `opacity`：绘制时逐层累积 `opacity * parentOpacity`
- `filter`：解析 `blur()` / `grayscale()` / `brightness()`

**表单控件**（`HtmlDocument` 内部自绘 + 状态管理）：
- `input[type=text/password]`：白底+边框，真实键盘输入，UTF-8 按字符处理退格
- `input[type=checkbox/radio]`：自绘方框/圆圈 + 对勾/圆点，点击切换（radio 同 name 互斥）
- `select`：当前项文字 + 三角箭头，点击循环切换
- `button` / `textarea`：自绘文字，点击触发回调

**关键 bug 修复**：
- **`SendNotify` 的 `wParam` 悬垂指针**：原先传局部 `CDuiString::GetData()`，
  Notify 里拿到已析构内存（表现为 `wParam=[` 空字符串）。改为存到成员变量 `m_sNotifyName`
- **源码文件编码**：新建 `.cpp/.h` 若为 UTF-8 无 BOM，MSVC 按 GBK 读会吞掉换行，
  报 `C3688`/`C2143`/`C2614` 等莫名其妙错误。统一加 BOM
- **绝对定位基准**：`position:absolute` 按最近 `position != static` 祖先的 padding box 定位

**验证**：
- 六个配置（SRelease/SDebug/Release × Win32/x64）全部编译通过
- `HtmlDemo` 实测：圆角窗口 + 渐变标题栏 + 三个 HTML 窗口按键（点击关闭按钮能真正关闭窗口）
- `HtmlFormTest` 25 项断言全部通过
