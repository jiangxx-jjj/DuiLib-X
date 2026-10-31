#include "StdAfx.h"
#include "js_bridge.h"
#include "el_form.h"
#include "../UIHtmlBox.h"
#include <windows.h>

namespace DuiLib
{
// 编码转换（UIHtmlBox.cpp 里的 static 函数在此不可见，这里自备一份）
inline CDuiString Utf8ToDui(const std::string& s)
{
    if (s.empty()) return _T("");
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (n <= 0) return _T("");
    std::wstring w(n, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return CDuiString(w.c_str());
}

inline std::string DuiToUtf8(const CDuiString& s)
{
    LPCTSTR p = s.GetData();
    if (!p || !p[0]) return std::string();
    int n = ::WideCharToMultiByte(CP_UTF8, 0, p, -1, NULL, 0, NULL, NULL);
    if (n <= 0) return std::string();
    std::string out(n - 1, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, p, -1, &out[0], n, NULL, NULL);
    return out;
}
}

namespace litehtml
{

// ============================ 生命周期 ============================

JsBridge::JsBridge()
    : m_ctx(NULL), m_host(NULL)
{
    m_ctx = duk_create_heap_default();
    if (m_ctx) RegisterApi();
}

JsBridge::~JsBridge()
{
    if (m_ctx) duk_destroy_heap(m_ctx);
}

JsBridge* JsBridge::FromCtx(duk_context* ctx)
{
    // JsBridge 实例指针存在全局 stash 里
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "\xFF" "jsbridge");
    void* p = duk_to_pointer(ctx, -1);
    duk_pop_2(ctx);
    return (JsBridge*)p;
}

// ============================ API 注册 ============================

void JsBridge::RegisterApi()
{
    if (!m_ctx) return;

    // 实例指针存入 stash
    duk_push_global_stash(m_ctx);
    duk_push_pointer(m_ctx, (void*)this);
    duk_put_prop_string(m_ctx, -2, "\xFF" "jsbridge");
    duk_pop(m_ctx);

    // 注册全局函数
    duk_push_c_function(m_ctx, DukAlert, 1);
    duk_put_global_string(m_ctx, "alert");

    duk_push_c_function(m_ctx, DukConfirm, 1);
    duk_put_global_string(m_ctx, "confirm");

    duk_push_c_function(m_ctx, DukGetValue, 1);
    duk_put_global_string(m_ctx, "get_value");

    duk_push_c_function(m_ctx, DukSetValue, 2);
    duk_put_global_string(m_ctx, "set_value");

    duk_push_c_function(m_ctx, DukSetChecked, 2);
    duk_put_global_string(m_ctx, "set_checked");

    duk_push_c_function(m_ctx, DukIsChecked, 1);
    duk_put_global_string(m_ctx, "is_checked");

    duk_push_c_function(m_ctx, DukCloseWindow, 0);
    duk_put_global_string(m_ctx, "close_window");

    // canvas 绘图 API
    duk_push_c_function(m_ctx, DukCanvasClear, 2);
    duk_put_global_string(m_ctx, "canvas_clear");
    duk_push_c_function(m_ctx, DukCanvasFillRect, 6);
    duk_put_global_string(m_ctx, "canvas_fill_rect");
    duk_push_c_function(m_ctx, DukCanvasDrawRect, 7);
    duk_put_global_string(m_ctx, "canvas_draw_rect");
    duk_push_c_function(m_ctx, DukCanvasDrawLine, 7);
    duk_put_global_string(m_ctx, "canvas_draw_line");
    duk_push_c_function(m_ctx, DukCanvasDrawCircle, 6);
    duk_put_global_string(m_ctx, "canvas_draw_circle");
    duk_push_c_function(m_ctx, DukCanvasDrawText, 6);
    duk_put_global_string(m_ctx, "canvas_draw_text");
}

// ============================ 桥接函数 ============================

duk_ret_t JsBridge::DukAlert(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* msg = duk_safe_to_string(ctx, 0);
    if (self && self->m_fnAlert) {
        self->m_fnAlert(msg ? msg : "");   // 弹窗转发到前端 DuiLib 窗口
    }
    return 0;
}

duk_ret_t JsBridge::DukConfirm(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* msg = duk_safe_to_string(ctx, 0);
    bool ok = false;
    if (self && self->m_fnConfirm) {
        ok = self->m_fnConfirm(msg ? msg : "");   // 询问框转发到前端
    }
    duk_push_boolean(ctx, ok ? 1 : 0);
    return 1;
}

duk_ret_t JsBridge::DukGetValue(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* name = duk_to_string(ctx, 0);
    std::string val;
    if (self && self->m_host && name) {
        DuiLib::CDuiString v = self->m_host->GetValue(DuiLib::Utf8ToDui(name));
        val = DuiLib::DuiToUtf8(v);
    }
    duk_push_string(ctx, val.c_str());
    return 1;
}

duk_ret_t JsBridge::DukSetValue(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* name = duk_to_string(ctx, 0);
    const char* val = duk_to_string(ctx, 1);
    if (self && self->m_host && name && val) {
        self->m_host->SetValue(DuiLib::Utf8ToDui(name), DuiLib::Utf8ToDui(val));
    }
    return 0;
}

duk_ret_t JsBridge::DukSetChecked(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* name = duk_to_string(ctx, 0);
    duk_bool_t checked = duk_to_boolean(ctx, 1);
    if (self && self->m_host && name) {
        self->m_host->SetChecked(DuiLib::Utf8ToDui(name), checked != 0);
    }
    return 0;
}

duk_ret_t JsBridge::DukIsChecked(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* name = duk_to_string(ctx, 0);
    bool checked = false;
    if (self && self->m_host && name) {
        checked = self->m_host->IsChecked(DuiLib::Utf8ToDui(name));
    }
    duk_push_boolean(ctx, checked ? 1 : 0);
    return 1;
}

duk_ret_t JsBridge::DukCloseWindow(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    if (self && self->m_host) {
        // 通过 DuiLib 通知机制关闭宿主窗口（发给后端逻辑处理）
        self->m_host->CloseHostWindow();
    }
    return 0;
}

// ============================ canvas 绘图桥接 ============================
// 都按第一个参数（id 或 name）找画布，然后调对应绘制方法

duk_ret_t JsBridge::DukCanvasClear(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* id = duk_to_string(ctx, 0);
    if (self && self->m_host) {
        el_canvas* pCanvas = self->m_host->FindCanvas(id);
        if (pCanvas) pCanvas->Clear(duk_to_string(ctx, 1));
    }
    return 0;
}

duk_ret_t JsBridge::DukCanvasFillRect(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* id = duk_to_string(ctx, 0);
    if (self && self->m_host) {
        el_canvas* pCanvas = self->m_host->FindCanvas(id);
        if (pCanvas) {
            pCanvas->FillRect((int)duk_to_int32(ctx, 1), (int)duk_to_int32(ctx, 2),
                              (int)duk_to_int32(ctx, 3), (int)duk_to_int32(ctx, 4),
                              duk_to_string(ctx, 5));
        }
    }
    return 0;
}

duk_ret_t JsBridge::DukCanvasDrawRect(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* id = duk_to_string(ctx, 0);
    if (self && self->m_host) {
        el_canvas* pCanvas = self->m_host->FindCanvas(id);
        if (pCanvas) {
            pCanvas->DrawRect((int)duk_to_int32(ctx, 1), (int)duk_to_int32(ctx, 2),
                              (int)duk_to_int32(ctx, 3), (int)duk_to_int32(ctx, 4),
                              duk_to_string(ctx, 5), (float)duk_to_number(ctx, 6));
        }
    }
    return 0;
}

duk_ret_t JsBridge::DukCanvasDrawLine(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* id = duk_to_string(ctx, 0);
    if (self && self->m_host) {
        el_canvas* pCanvas = self->m_host->FindCanvas(id);
        if (pCanvas) {
            pCanvas->DrawLine((int)duk_to_int32(ctx, 1), (int)duk_to_int32(ctx, 2),
                              (int)duk_to_int32(ctx, 3), (int)duk_to_int32(ctx, 4),
                              duk_to_string(ctx, 5), (float)duk_to_number(ctx, 6));
        }
    }
    return 0;
}

duk_ret_t JsBridge::DukCanvasDrawCircle(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* id = duk_to_string(ctx, 0);
    if (self && self->m_host) {
        el_canvas* pCanvas = self->m_host->FindCanvas(id);
        if (pCanvas) {
            pCanvas->DrawCircle((int)duk_to_int32(ctx, 1), (int)duk_to_int32(ctx, 2),
                                (float)duk_to_number(ctx, 3),
                                duk_to_string(ctx, 4),
                                duk_to_boolean(ctx, 5) != 0);
        }
    }
    return 0;
}

duk_ret_t JsBridge::DukCanvasDrawText(duk_context* ctx)
{
    JsBridge* self = FromCtx(ctx);
    const char* id = duk_to_string(ctx, 0);
    if (self && self->m_host) {
        el_canvas* pCanvas = self->m_host->FindCanvas(id);
        if (pCanvas) {
            pCanvas->DrawText(duk_to_string(ctx, 1),
                              (int)duk_to_int32(ctx, 2), (int)duk_to_int32(ctx, 3),
                              duk_to_string(ctx, 4), (float)duk_to_number(ctx, 5));
        }
    }
    return 0;
}

// ============================ 执行 ============================

bool JsBridge::Eval(const std::string& jsCode)
{
    if (!m_ctx) return false;
    m_lastError.clear();

    duk_int_t rc = duk_peval_string(m_ctx, jsCode.c_str());
    if (rc != 0) {
        m_lastError = duk_safe_to_string(m_ctx, -1);
        duk_pop(m_ctx);
        return false;
    }
    duk_pop(m_ctx);   // 结果
    return true;
}

std::string JsBridge::EvalString(const std::string& jsCode)
{
    if (!m_ctx) return "";
    m_lastError.clear();

    duk_int_t rc = duk_peval_string(m_ctx, jsCode.c_str());
    if (rc != 0) {
        m_lastError = duk_safe_to_string(m_ctx, -1);
        duk_pop(m_ctx);
        return "";
    }
    const char* result = duk_safe_to_string(m_ctx, -1);
    std::string out = result ? result : "";
    duk_pop(m_ctx);
    return out;
}

} // namespace litehtml
