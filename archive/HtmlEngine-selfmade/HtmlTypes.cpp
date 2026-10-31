#include "StdAfx.h"
#include "HtmlTypes.h"

namespace HtmlEngine
{

// ============================ 颜色名表 ============================

struct NamedColor { const char* name; unsigned char r, g, b; };

static const NamedColor g_namedColors[] =
{
    { "transparent",      0,   0,   0   },
    { "black",            0,   0,   0   },
    { "silver",           192, 192, 192 },
    { "gray",             128, 128, 128 },
    { "grey",             128, 128, 128 },
    { "white",            255, 255, 255 },
    { "maroon",           128, 0,   0   },
    { "red",              255, 0,   0   },
    { "purple",           128, 0,   128 },
    { "fuchsia",          255, 0,   255 },
    { "magenta",          255, 0,   255 },
    { "green",            0,   128, 0   },
    { "lime",             0,   255, 0   },
    { "olive",            128, 128, 0   },
    { "yellow",           255, 255, 0   },
    { "navy",             0,   0,   128 },
    { "blue",             0,   0,   255 },
    { "teal",             0,   128, 128 },
    { "aqua",             0,   255, 255 },
    { "cyan",             0,   255, 255 },
    { "orange",           255, 165, 0   },
    { "aliceblue",        240, 248, 255 },
    { "antiquewhite",     250, 235, 215 },
    { "aquamarine",       127, 255, 212 },
    { "azure",            240, 255, 255 },
    { "beige",            245, 245, 220 },
    { "bisque",           255, 228, 196 },
    { "blanchedalmond",   255, 235, 205 },
    { "blueviolet",       138, 43,  226 },
    { "brown",            165, 42,  42  },
    { "burlywood",        222, 184, 135 },
    { "cadetblue",        95,  158, 160 },
    { "chartreuse",       127, 255, 0   },
    { "chocolate",        210, 105, 30  },
    { "coral",            255, 127, 80  },
    { "cornflowerblue",   100, 149, 237 },
    { "cornsilk",         255, 248, 220 },
    { "crimson",          220, 20,  60  },
    { "darkblue",         0,   0,   139 },
    { "darkcyan",         0,   139, 139 },
    { "darkgoldenrod",    184, 134, 11  },
    { "darkgray",         169, 169, 169 },
    { "darkgrey",         169, 169, 169 },
    { "darkgreen",        0,   100, 0   },
    { "darkkhaki",        189, 183, 107 },
    { "darkmagenta",      139, 0,   139 },
    { "darkolivegreen",   85,  107, 47  },
    { "darkorange",       255, 140, 0   },
    { "darkorchid",       153, 50,  204 },
    { "darkred",          139, 0,   0   },
    { "darksalmon",       233, 150, 122 },
    { "darkseagreen",     143, 188, 143 },
    { "darkslateblue",    72,  61,  139 },
    { "darkslategray",    47,  79,  79  },
    { "darkslategrey",    47,  79,  79  },
    { "darkturquoise",    0,   206, 209 },
    { "darkviolet",       148, 0,   211 },
    { "deeppink",         255, 20,  147 },
    { "deepskyblue",      0,   191, 255 },
    { "dimgray",          105, 105, 105 },
    { "dimgrey",          105, 105, 105 },
    { "dodgerblue",       30,  144, 255 },
    { "firebrick",        178, 34,  34  },
    { "floralwhite",      255, 250, 240 },
    { "forestgreen",      34,  139, 34  },
    { "gainsboro",        220, 220, 220 },
    { "ghostwhite",       248, 248, 255 },
    { "gold",             255, 215, 0   },
    { "goldenrod",        218, 165, 32  },
    { "greenyellow",      173, 255, 47  },
    { "honeydew",         240, 255, 240 },
    { "hotpink",          255, 105, 180 },
    { "indianred",        205, 92,  92  },
    { "indigo",           75,  0,   130 },
    { "ivory",            255, 255, 240 },
    { "khaki",            240, 230, 140 },
    { "lavender",         230, 230, 250 },
    { "lavenderblush",    255, 240, 245 },
    { "lawngreen",        124, 252, 0   },
    { "lemonchiffon",     255, 250, 205 },
    { "lightblue",        173, 216, 230 },
    { "lightcoral",       240, 128, 128 },
    { "lightcyan",        224, 255, 255 },
    { "lightgoldenrodyellow", 250, 250, 210 },
    { "lightgray",        211, 211, 211 },
    { "lightgrey",        211, 211, 211 },
    { "lightgreen",       144, 238, 144 },
    { "lightpink",        255, 182, 193 },
    { "lightsalmon",      255, 160, 122 },
    { "lightseagreen",    32,  178, 170 },
    { "lightskyblue",     135, 206, 250 },
    { "lightslategray",   119, 136, 153 },
    { "lightslategrey",   119, 136, 153 },
    { "lightsteelblue",   176, 196, 222 },
    { "lightyellow",      255, 255, 224 },
    { "limegreen",        50,  205, 50  },
    { "linen",            250, 240, 230 },
    { "mediumaquamarine", 102, 205, 170 },
    { "mediumblue",       0,   0,   205 },
    { "mediumorchid",     186, 85,  211 },
    { "mediumpurple",     147, 112, 219 },
    { "mediumseagreen",   60,  179, 113 },
    { "mediumslateblue",  123, 104, 238 },
    { "mediumspringgreen",0,   250, 154 },
    { "mediumturquoise",  72,  209, 204 },
    { "mediumvioletred",  199, 21,  133 },
    { "midnightblue",     25,  25,  112 },
    { "mintcream",        245, 255, 250 },
    { "mistyrose",        255, 228, 225 },
    { "moccasin",         255, 228, 181 },
    { "navajowhite",      255, 222, 173 },
    { "oldlace",          253, 245, 230 },
    { "olivedrab",        107, 142, 35  },
    { "orangered",        255, 69,  0   },
    { "orchid",           218, 112, 214 },
    { "palegoldenrod",    238, 232, 170 },
    { "palegreen",        152, 251, 152 },
    { "paleturquoise",    175, 238, 238 },
    { "palevioletred",    219, 112, 147 },
    { "papayawhip",       255, 239, 213 },
    { "peachpuff",        255, 218, 185 },
    { "peru",             205, 133, 63  },
    { "pink",             255, 192, 203 },
    { "plum",             221, 160, 221 },
    { "powderblue",       176, 224, 230 },
    { "rosybrown",        188, 143, 143 },
    { "royalblue",        65,  105, 225 },
    { "saddlebrown",      139, 69,  19  },
    { "salmon",           250, 128, 114 },
    { "sandybrown",       244, 164, 96  },
    { "seagreen",         46,  139, 87  },
    { "seashell",         255, 245, 238 },
    { "sienna",           160, 82,  45  },
    { "skyblue",          135, 206, 235 },
    { "slateblue",        106, 90,  205 },
    { "slategray",        112, 128, 144 },
    { "slategrey",        112, 128, 144 },
    { "snow",             255, 250, 250 },
    { "springgreen",      0,   255, 127 },
    { "steelblue",        70,  130, 180 },
    { "tan",              210, 180, 140 },
    { "thistle",          216, 191, 216 },
    { "tomato",           255, 99,  71  },
    { "turquoise",        64,  224, 208 },
    { "violet",           238, 130, 238 },
    { "wheat",            245, 222, 179 },
    { "whitesmoke",       245, 245, 245 },
    { "yellowgreen",      154, 205, 50  },
    { "rebeccapurple",    102, 51,  153 },
};

bool Color::FromName(const std::string& name, Color& out)
{
    std::string n = ToLower(Trim(name));
    if (n == "transparent") { out = Color(0, 0, 0, 0); return true; }

    for (size_t i = 0; i < sizeof(g_namedColors) / sizeof(g_namedColors[0]); i++) {
        if (n == g_namedColors[i].name) {
            out = Color(g_namedColors[i].r, g_namedColors[i].g, g_namedColors[i].b, 255);
            return true;
        }
    }
    return false;
}

Color Color::FromString(const std::string& s)
{
    std::string v = Trim(s);
    if (v.empty()) return Color::Transparent();

    // #RGB / #RRGGBB / #RRGGBBAA
    if (v[0] == '#') {
        std::string hex = v.substr(1);
        if (hex.size() == 3) {
            int r = (int)strtol(hex.substr(0, 1).c_str(), NULL, 16);
            int g = (int)strtol(hex.substr(1, 1).c_str(), NULL, 16);
            int b = (int)strtol(hex.substr(2, 1).c_str(), NULL, 16);
            return Color((unsigned char)(r * 17), (unsigned char)(g * 17), (unsigned char)(b * 17), 255);
        }
        if (hex.size() == 4) {
            int r = (int)strtol(hex.substr(0, 1).c_str(), NULL, 16);
            int g = (int)strtol(hex.substr(1, 1).c_str(), NULL, 16);
            int b = (int)strtol(hex.substr(2, 1).c_str(), NULL, 16);
            int a = (int)strtol(hex.substr(3, 1).c_str(), NULL, 16);
            return Color((unsigned char)(r * 17), (unsigned char)(g * 17),
                         (unsigned char)(b * 17), (unsigned char)(a * 17));
        }
        if (hex.size() == 6) {
            int r = (int)strtol(hex.substr(0, 2).c_str(), NULL, 16);
            int g = (int)strtol(hex.substr(2, 2).c_str(), NULL, 16);
            int b = (int)strtol(hex.substr(4, 2).c_str(), NULL, 16);
            return Color((unsigned char)r, (unsigned char)g, (unsigned char)b, 255);
        }
        if (hex.size() == 8) {
            int r = (int)strtol(hex.substr(0, 2).c_str(), NULL, 16);
            int g = (int)strtol(hex.substr(2, 2).c_str(), NULL, 16);
            int b = (int)strtol(hex.substr(4, 2).c_str(), NULL, 16);
            int a = (int)strtol(hex.substr(6, 2).c_str(), NULL, 16);
            return Color((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
        }
        return Color::Transparent();
    }

    // rgb() / rgba()
    std::string low = ToLower(v);
    if (low.find("rgba(") == 0 || low.find("rgb(") == 0) {
        size_t p1 = v.find('(');
        size_t p2 = v.find(')');
        if (p1 != std::string::npos && p2 != std::string::npos) {
            std::string inner = v.substr(p1 + 1, p2 - p1 - 1);
            std::vector<std::string> parts = Split(inner, ',');
            if (parts.size() >= 3) {
                int r = (int)atof(Trim(parts[0]).c_str());
                int g = (int)atof(Trim(parts[1]).c_str());
                int b = (int)atof(Trim(parts[2]).c_str());
                int a = 255;
                if (parts.size() >= 4) {
                    std::string as = Trim(parts[3]);
                    if (as.find('%') != std::string::npos) a = (int)(atof(as.c_str()) * 2.55);
                    else a = (int)(atof(as.c_str()) * 255.0);
                }
                if (r < 0) r = 0; if (r > 255) r = 255;
                if (g < 0) g = 0; if (g > 255) g = 255;
                if (b < 0) b = 0; if (b > 255) b = 255;
                if (a < 0) a = 0; if (a > 255) a = 255;
                return Color((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
            }
        }
    }

    // hsl() / hsla()
    if (low.find("hsla(") == 0 || low.find("hsl(") == 0) {
        size_t p1 = v.find('(');
        size_t p2 = v.find(')');
        if (p1 != std::string::npos && p2 != std::string::npos) {
            std::string inner = v.substr(p1 + 1, p2 - p1 - 1);
            std::vector<std::string> parts = Split(inner, ',');
            if (parts.size() >= 3) {
                float h = (float)atof(Trim(parts[0]).c_str());
                float sat = (float)atof(Trim(parts[1]).c_str()) / 100.0f;
                float lig = (float)atof(Trim(parts[2]).c_str()) / 100.0f;
                float alpha = 1.0f;
                if (parts.size() >= 4) alpha = (float)atof(Trim(parts[3]).c_str());

                // HSL → RGB
                float c = (1.0f - fabsf(2.0f * lig - 1.0f)) * sat;
                float hp = h / 60.0f;
                float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
                float m = lig - c / 2.0f;
                float rr = 0, gg = 0, bb = 0;
                if      (hp < 1) { rr = c; gg = x; bb = 0; }
                else if (hp < 2) { rr = x; gg = c; bb = 0; }
                else if (hp < 3) { rr = 0; gg = c; bb = x; }
                else if (hp < 4) { rr = 0; gg = x; bb = c; }
                else if (hp < 5) { rr = x; gg = 0; bb = c; }
                else             { rr = c; gg = 0; bb = x; }
                return Color((unsigned char)((rr + m) * 255.0f),
                             (unsigned char)((gg + m) * 255.0f),
                             (unsigned char)((bb + m) * 255.0f),
                             (unsigned char)(alpha * 255.0f));
            }
        }
    }

    // 颜色名
    Color named;
    if (FromName(v, named)) return named;

    return Color::Transparent();
}

bool Color::IsColorString(const std::string& s)
{
    std::string v = Trim(s);
    if (v.empty()) return false;
    if (v[0] == '#') {
        size_t n = v.size() - 1;
        return n == 3 || n == 4 || n == 6 || n == 8;
    }
    std::string low = ToLower(v);
    if (low.find("rgb(") == 0 || low.find("rgba(") == 0 ||
        low.find("hsl(") == 0 || low.find("hsla(") == 0) {
        return true;
    }
    Color c;
    return FromName(v, c);
}

// ============================ 长度解析 ============================

Length Length::FromString(const std::string& s)
{
    std::string v = Trim(s);
    if (v.empty()) return Length(0, AUTO);

    std::string low = ToLower(v);
    if (low == "auto")   return Length(0, AUTO);
    if (low == "none")   return Length(0, NONE);
    if (low == "normal") return Length(0, AUTO);

    // 提取数字部分
    size_t i = 0;
    if (v[i] == '+' || v[i] == '-') i++;
    while (i < v.size() && (isdigit((unsigned char)v[i]) || v[i] == '.')) i++;
    if (i == 0) return Length(0, AUTO);

    float num = (float)atof(v.substr(0, i).c_str());
    std::string unitStr = Trim(v.substr(i));
    std::string unitLow = ToLower(unitStr);

    if (unitLow.empty() || unitLow == "px") return Length(num, PX);
    if (unitLow == "%")                     return Length(num, PERCENT);
    if (unitLow == "em")                    return Length(num, EM);
    if (unitLow == "rem")                   return Length(num, REM);
    if (unitLow == "pt")                    return Length(num, PT);
    if (unitLow == "vw")                    return Length(num, VW);
    if (unitLow == "vh")                    return Length(num, VH);
    // 其它单位（cm/mm/in/ex/ch）统一按 px 处理
    return Length(num, PX);
}

float Length::Resolve(float base, float fontSize, float rootFontSize) const
{
    switch (unit) {
    case PX:      return value;
    case PT:      return value * 96.0f / 72.0f;
    case PERCENT: return base * value / 100.0f;
    case EM:      return fontSize * value;
    case REM:     return rootFontSize * value;
    case VW:      return base * value / 100.0f;
    case VH:      return base * value / 100.0f;
    case AUTO:
    case NONE:
    default:      return 0.0f;
    }
}

// ============================ 字符串工具 ============================

std::string Trim(const std::string& s)
{
    if (s.empty()) return s;
    size_t b = 0, e = s.size();
    while (b < e && isspace((unsigned char)s[b])) b++;
    while (e > b && isspace((unsigned char)s[e - 1])) e--;
    return s.substr(b, e - b);
}

std::string ToLower(const std::string& s)
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); i++) r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

std::vector<std::string> Split(const std::string& s, char delim)
{
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == delim) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

std::vector<std::string> SplitTopLevel(const std::string& s, char delim)
{
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    for (size_t i = 0; i < s.size(); i++) {
        char ch = s[i];
        if (ch == '(') depth++;
        else if (ch == ')') { if (depth > 0) depth--; }

        if (ch == delim && depth == 0) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += ch;
        }
    }
    out.push_back(cur);
    return out;
}

std::string ReplaceAll(const std::string& s, const std::string& from, const std::string& to)
{
    if (from.empty()) return s;
    std::string out;
    size_t pos = 0;
    while (true) {
        size_t p = s.find(from, pos);
        if (p == std::string::npos) {
            out += s.substr(pos);
            break;
        }
        out += s.substr(pos, p - pos);
        out += to;
        pos = p + from.size();
    }
    return out;
}

std::string FloatToString(float v)
{
    char buf[64];
    _snprintf(buf, sizeof(buf), "%.2f", v);
    buf[sizeof(buf) - 1] = 0;
    // 去掉末尾多余的 0
    std::string s = buf;
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s[s.size() - 1] == '0') s.erase(s.size() - 1);
        if (!s.empty() && s[s.size() - 1] == '.') s.erase(s.size() - 1);
    }
    return s;
}

std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return std::wstring();
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (n <= 0) return std::wstring();
    std::wstring w(n, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return std::string();
    int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    if (n <= 0) return std::string();
    std::string s(n, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, NULL, NULL);
    return s;
}

} // namespace HtmlEngine
