#ifndef LH_JS_BRIDGE_H
#define LH_JS_BRIDGE_H

// ==========================================================================
//  js_bridge.h — 后端 JS 引擎（Duktape）绑定层
//
//  架构：
//  · JS 逻辑在 C++ 后端执行（Duktape，ANSI C，MIT 许可）
//  · 前端 HTML 里的 <script> 标签内容由 DuiLib 提取后交给本类执行
//  · JS 通过桥接函数操作页面：alert()/confirm()/get_value()/set_value()…
//  · 弹窗（alert/confirm）不在后端显示，而是回调到前端 DuiLib 窗口弹出
// ==========================================================================

#include "duktape.h"
#include <string>
#include <functional>

namespace DuiLib { class CHtmlBoxUI; }   // 前向声明（DuiLib 控件，弱引用）

namespace litehtml
{

class JsBridge
{
public:
    JsBridge();
    ~JsBridge();

    // 绑定宿主控件（供 JS 读写表单值）
    void BindHost(DuiLib::CHtmlBoxUI* host) { m_host = host; }

    // 执行一段 JS 代码（UTF-8），返回是否成功
    bool Eval(const std::string& jsCode);

    // 执行并返回字符串结果（失败返回空）
    std::string EvalString(const std::string& jsCode);

    // 取最后的错误信息
    const std::string& LastError() const { return m_lastError; }

    // ---- JS 可调用的桥接函数（注册到全局对象）----
    // alert(msg)            → 回调 m_fnAlert（前端弹窗）
    // confirm(msg) → bool   → 回调 m_fnConfirm（前端弹窗询问）
    // get_value(name)       → 读表单控件值
    // set_value(name, val)  → 写表单控件值
    // set_checked(name, b)  → 勾选/取消勾选
    // is_checked(name)      → 查询勾选状态
    // close_window()        → 关闭宿主窗口
    void RegisterApi();

    // ---- 弹窗回调（由 DuiLib 注入，在前端显示）----
    void SetAlertCallback(std::function<void(const std::string& msg)> fn) { m_fnAlert = fn; }
    void SetConfirmCallback(std::function<bool(const std::string& msg)> fn) { m_fnConfirm = fn; }

private:
    duk_context* m_ctx;
    DuiLib::CHtmlBoxUI*  m_host;
    std::string  m_lastError;

    std::function<void(const std::string&)>     m_fnAlert;
    std::function<bool(const std::string&)>     m_fnConfirm;

    // duk 函数指针：alert/confirm 桥接
    static duk_ret_t DukAlert(duk_context* ctx);
    static duk_ret_t DukConfirm(duk_context* ctx);
    static duk_ret_t DukGetValue(duk_context* ctx);
    static duk_ret_t DukSetValue(duk_context* ctx);
    static duk_ret_t DukSetChecked(duk_context* ctx);
    static duk_ret_t DukIsChecked(duk_context* ctx);
    static duk_ret_t DukCloseWindow(duk_context* ctx);

    // 从 ctx 取 JsBridge 实例（存于全局 stash）
    static JsBridge* FromCtx(duk_context* ctx);
};

} // namespace litehtml

#endif // LH_JS_BRIDGE_H
