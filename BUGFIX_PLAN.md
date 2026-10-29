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

## 新增功能：HTML 界面（HtmlBox）
- 集成 litehtml v0.9（BSD-3）实现零内核 HTML/CSS 渲染
- CHtmlBoxUI 控件：SetHtml/SetHtmlFromFile/GetValue/SetValue/IsChecked/SetChecked
- CDuiHtmlContainer 绘制回调（字体/文本/背景/边框/图片/裁剪/DPI）
- 表单控件映射原生控件、链接点击事件转发