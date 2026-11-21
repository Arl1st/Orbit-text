#include <FFGLSDK.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <chrono>

using namespace ffglex;

namespace
{
enum ParamIndex : unsigned int
{
    TEXT_1 = 0, SPEED_1, SIZE_1,
    TEXT_2, SPEED_2, SIZE_2,
    TEXT_3, SPEED_3, SIZE_3,
    PATH_MODE, COLOR_R, COLOR_G, COLOR_B, GLOW
};

static const char* kVertexShader = R"(
#version 410 core
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vUV;
out vec2 uv;
void main() { gl_Position = vPosition; uv = vUV; }
)";

static const char* kFragmentShader = R"(
#version 410 core
uniform sampler2D textTexture;
uniform vec2 texelSize;
uniform vec3 textColor;
uniform float glowAmount;
in vec2 uv;
out vec4 fragColor;
void main()
{
    float a = texture(textTexture, vec2(uv.x, 1.0 - uv.y)).a;
    float g = 0.0;
    for (int x = -2; x <= 2; ++x)
        for (int y = -2; y <= 2; ++y)
            g = max(g, texture(textTexture, vec2(uv.x, 1.0 - uv.y) + vec2(x,-y) * texelSize * 2.0).a);
    float alpha = max(a, g * glowAmount);
    fragColor = vec4(textColor, alpha);
}
)";

std::wstring Utf8ToWide(const std::string& s)
{
    std::wstring out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i++];
        unsigned int cp = c;
        if ((c & 0xE0) == 0xC0 && i < s.size()) {
            cp = ((c & 0x1F) << 6) | ((unsigned char)s[i++] & 0x3F);
        } else if ((c & 0xF0) == 0xE0 && i + 1 < s.size()) {
            cp = ((c & 0x0F) << 12) | (((unsigned char)s[i++] & 0x3F) << 6);
            cp |= ((unsigned char)s[i++] & 0x3F);
        } else if ((c & 0xF8) == 0xF0 && i + 2 < s.size()) {
            cp = ((c & 0x07) << 18) | (((unsigned char)s[i++] & 0x3F) << 12);
            cp |= (((unsigned char)s[i++] & 0x3F) << 6) | ((unsigned char)s[i++] & 0x3F);
        }
        if (sizeof(wchar_t) == 2 && cp > 0xFFFF) {
            cp -= 0x10000;
            out.push_back((wchar_t)(0xD800 + (cp >> 10)));
            out.push_back((wchar_t)(0xDC00 + (cp & 0x3FF)));
        } else {
            out.push_back((wchar_t)cp);
        }
    }
    return out;
}

struct PathPoint { float x, y, tx, ty; };

PathPoint RoundedRectPoint(float d, float cx, float cy, float rx, float ry, float r)
{
    r = std::max(2.0f, std::min(r, std::min(rx, ry) - 1.0f));
    const float top = 2.0f * (rx - r);
    const float side = 2.0f * (ry - r);
    const float arc = 1.57079632679f * r;
    const float total = 2.0f * top + 2.0f * side + 4.0f * arc;
    d = std::fmod(d, total);
    if (d < 0.0f) d += total;

    if (d < top) return {cx-rx+r+d, cy-ry, 1, 0};
    d -= top;
    if (d < arc) {
        float a = -1.57079632679f + d/r;
        return {cx+rx-r+r*std::cos(a), cy-ry+r+r*std::sin(a), -std::sin(a), std::cos(a)};
    }
    d -= arc;
    if (d < side) return {cx+rx, cy-ry+r+d, 0, 1};
    d -= side;
    if (d < arc) {
        float a = d/r;
        return {cx+rx-r+r*std::cos(a), cy+ry-r+r*std::sin(a), -std::sin(a), std::cos(a)};
    }
    d -= arc;
    if (d < top) return {cx+rx-r-d, cy+ry, -1, 0};
    d -= top;
    if (d < arc) {
        float a = 1.57079632679f + d/r;
        return {cx-rx+r+r*std::cos(a), cy+ry-r+r*std::sin(a), -std::sin(a), std::cos(a)};
    }
    d -= arc;
    if (d < side) return {cx-rx, cy+ry-r-d, 0, -1};
    d -= side;
    float a = 3.14159265359f + d/r;
    return {cx-rx+r+r*std::cos(a), cy-ry+r+r*std::sin(a), -std::sin(a), std::cos(a)};
}

class OrbitText final : public CFFGLPlugin
{
public:
    OrbitText() : CFFGLPlugin(false)
    {
        SetMinInputs(0);
        SetMaxInputs(0);
        SetParamInfo(TEXT_1, "Text 1", FF_TYPE_TEXT, "ORBIT TEXT");
        SetParamInfo(SPEED_1, "Speed 1", FF_TYPE_STANDARD, 0.62f);
        SetParamInfo(SIZE_1, "Size 1", FF_TYPE_STANDARD, 0.45f);
        SetParamInfo(TEXT_2, "Text 2", FF_TYPE_TEXT, "RESOLUME ARENA");
        SetParamInfo(SPEED_2, "Speed 2", FF_TYPE_STANDARD, 0.38f);
        SetParamInfo(SIZE_2, "Size 2", FF_TYPE_STANDARD, 0.34f);
        SetParamInfo(TEXT_3, "Text 3", FF_TYPE_TEXT, "LIVE VISUALS");
        SetParamInfo(SPEED_3, "Speed 3", FF_TYPE_STANDARD, 0.56f);
        SetParamInfo(SIZE_3, "Size 3", FF_TYPE_STANDARD, 0.28f);
        SetOptionParamInfo(PATH_MODE, "Path", 3, 0);
        SetParamElementInfo(PATH_MODE, 0, "Circle", 0.0f);
        SetParamElementInfo(PATH_MODE, 1, "Rounded square", 1.0f);
        SetParamElementInfo(PATH_MODE, 2, "Rounded rectangle", 2.0f);
        SetParamInfo(COLOR_R, "Red", FF_TYPE_RED, 1.0f);
        SetParamInfo(COLOR_G, "Green", FF_TYPE_GREEN, 1.0f);
        SetParamInfo(COLOR_B, "Blue", FF_TYPE_BLUE, 1.0f);
        SetParamInfo(GLOW, "Glow", FF_TYPE_STANDARD, 0.35f);
        texts[0] = "ORBIT TEXT";
        texts[1] = "RESOLUME ARENA";
        texts[2] = "LIVE VISUALS";
        floats[SPEED_1] = 0.62f; floats[SIZE_1] = 0.45f;
        floats[SPEED_2] = 0.38f; floats[SIZE_2] = 0.34f;
        floats[SPEED_3] = 0.56f; floats[SIZE_3] = 0.28f;
        floats[PATH_MODE] = 0.0f;
        floats[COLOR_R] = floats[COLOR_G] = floats[COLOR_B] = 1.0f;
        floats[GLOW] = 0.35f;
        SetTimeSupported(true);
    }

    FFResult InitGL(const FFGLViewportStruct* vp) override
    {
        if (!shader.Compile(kVertexShader, kFragmentShader)) return FF_FAIL;
        if (!quad.Initialise()) { shader.FreeGLResources(); return FF_FAIL; }
        if (!CreateCanvas(vp->width, vp->height)) { DeInitGL(); return FF_FAIL; }
        ScopedShaderBinding bind(shader.GetGLID());
        samplerLoc = shader.FindUniform("textTexture");
        texelLoc = shader.FindUniform("texelSize");
        colorLoc = shader.FindUniform("textColor");
        glowLoc = shader.FindUniform("glowAmount");
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        glUniform1i(samplerLoc, 0);
        return CFFGLPlugin::InitGL(vp);
    }

    FFResult ProcessOpenGL(ProcessOpenGLStruct*) override
    {
        if (!dc || !pixels || !texture) return FF_FAIL;
        RenderText();
        glActiveTexture(GL_TEXTURE0);
        GLint previous = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        {
            ScopedShaderBinding bind(shader.GetGLID());
            glUniform1i(samplerLoc, 0);
            glUniform2f(texelLoc, 1.0f / (float)width, 1.0f / (float)height);
            glUniform3f(colorLoc, floats[COLOR_R], floats[COLOR_G], floats[COLOR_B]);
            glUniform1f(glowLoc, 0.35f + floats[GLOW] * 1.65f);
            quad.Draw();
        }
        glBindTexture(GL_TEXTURE_2D, (GLuint)previous);
        return FF_SUCCESS;
    }

    FFResult DeInitGL() override
    {
        if (texture) { glDeleteTextures(1, &texture); texture = 0; }
        shader.FreeGLResources();
        quad.Release();
        DestroyCanvas();
        return FF_SUCCESS;
    }

    unsigned int Resize(const FFGLViewportStruct* vp) override
    {
        CFFGLPlugin::Resize(vp);
        if (!CreateCanvas(vp->width, vp->height)) return FF_FAIL;
        if (texture) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        }
        return FF_SUCCESS;
    }

    FFResult SetFloatParameter(unsigned int index, float value) override
    {
        if (index >= SPEED_1 && index <= GLOW && index != TEXT_2 && index != TEXT_3) {
            floats[index] = std::max(0.0f, std::min(1.0f, value));
            return FF_SUCCESS;
        }
        return FF_FAIL;
    }

    float GetFloatParameter(unsigned int index) override
    {
        return index < 14 ? floats[index] : 0.0f;
    }

    FFResult SetTextParameter(unsigned int index, const char* value) override
    {
        if (index != TEXT_1 && index != TEXT_2 && index != TEXT_3) return FF_FAIL;
        texts[index == TEXT_1 ? 0 : (index == TEXT_2 ? 1 : 2)] = value ? value : "";
        return FF_SUCCESS;
    }

    char* GetTextParameter(unsigned int index) override
    {
        if (index != TEXT_1 && index != TEXT_2 && index != TEXT_3) return (char*)FF_FAIL;
        return const_cast<char*>(texts[index == TEXT_1 ? 0 : (index == TEXT_2 ? 1 : 2)].c_str());
    }

private:
    bool CreateCanvas(int w, int h)
    {
        DestroyCanvas();
        width = std::max(64, std::min(w, 4096));
        height = std::max(64, std::min(h, 4096));
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        dc = CreateCompatibleDC(nullptr);
        if (!dc) return false;
        bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void**)&pixels, nullptr, 0);
        if (!bitmap || !pixels) { DestroyCanvas(); return false; }
        oldBitmap = (HBITMAP)SelectObject(dc, bitmap);
        SetGraphicsMode(dc, GM_ADVANCED);
        return true;
    }

    void DestroyCanvas()
    {
        if (dc && oldBitmap) SelectObject(dc, oldBitmap);
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
        dc = nullptr; bitmap = nullptr; oldBitmap = nullptr; pixels = nullptr;
    }

    void DrawLine(int line, float now)
    {
        std::wstring text = Utf8ToWide(texts[line]);
        if (text.empty()) return;
        float sizeValue = floats[SIZE_1 + line * 3];
        int fontPx = (int)(22.0f + sizeValue * 78.0f);
        HFONT font = CreateFontW(-fontPx, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        if (!font) return;
        HGDIOBJ oldFont = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255,255,255));

        std::vector<int> advances(text.size());
        SIZE extent{};
        int total = 0;
        for (size_t i = 0; i < text.size(); ++i) {
            GetTextExtentPoint32W(dc, &text[i], 1, &extent);
            advances[i] = std::max(1L, extent.cx + 2);
            total += advances[i];
        }
        if (total > 0) {
            float speed = (floats[SPEED_1 + line * 3] - 0.5f) * 360.0f;
            float travel = std::fmod(now * speed, (float)std::max(width, height) * 8.0f + total);
            float offset = travel;
            int mode = (int)(floats[PATH_MODE] + 0.5f);
            float cx = width * 0.5f, cy = height * 0.5f;
            float baseRx = mode == 0 ? std::min(width,height)*0.34f : width*0.42f;
            float baseRy = mode == 0 ? std::min(width,height)*0.34f : (mode == 1 ? height*0.42f : height*0.32f);
            float lane = (line - 1) * (fontPx + 12.0f);
            float rx = std::max(30.0f, baseRx - lane);
            float ry = std::max(30.0f, baseRy - lane);
            float radius = std::max(12.0f, std::min(rx,ry)*0.16f);
            float perimeter = mode == 0 ? 6.2831853f * std::sqrt((rx*rx + ry*ry)*0.5f)
                : 2.0f*(rx-radius)*2.0f + 2.0f*(ry-radius)*2.0f + 6.2831853f*radius;

            float cursor = -offset;
            while (cursor < perimeter) cursor += total;
            cursor = std::fmod(cursor, (float)total);
            for (size_t i = 0; i < text.size(); ++i) {
                float dist = cursor;
                PathPoint p{};
                if (mode == 0) {
                    float a = dist / perimeter * 6.2831853f;
                    p = {cx + rx*std::cos(a), cy + ry*std::sin(a),
                         -std::sin(a), std::cos(a)};
                } else {
                    p = RoundedRectPoint(dist, cx, cy, rx, ry, radius);
                }
                float nx = -p.ty, ny = p.tx;
                XFORM xf{};
                xf.eM11 = p.tx; xf.eM12 = p.ty;
                xf.eM21 = nx; xf.eM22 = ny;
                xf.eDx = p.x - nx * fontPx * 0.55f;
                xf.eDy = p.y - ny * fontPx * 0.55f;
                SetWorldTransform(dc, &xf);
                TextOutW(dc, 0, 0, &text[i], 1);
                cursor += advances[i];
                if (cursor >= perimeter) cursor -= perimeter;
            }
        }
        ModifyWorldTransform(dc, nullptr, MWT_IDENTITY);
        SelectObject(dc, oldFont);
        DeleteObject(font);
    }

    void RenderText()
    {
        std::memset(pixels, 0, (size_t)width * (size_t)height * 4);
        static const auto epoch = std::chrono::steady_clock::now();
        float now = std::chrono::duration<float>(std::chrono::steady_clock::now() - epoch).count();
        for (int line = 0; line < 3; ++line) DrawLine(line, now);
        // GDI leaves the alpha byte undefined. Convert the white glyph coverage to alpha.
        for (size_t i = 0, n = (size_t)width * (size_t)height; i < n; ++i) {
            unsigned char* px = pixels + i*4;
            unsigned char a = std::max(px[0], std::max(px[1], px[2]));
            px[0] = px[1] = px[2] = 255;
            px[3] = a;
        }
    }

    FFGLShader shader;
    FFGLScreenQuad quad;
    GLuint texture = 0;
    GLint samplerLoc = -1, texelLoc = -1, colorLoc = -1, glowLoc = -1;
    int width = 0, height = 0;
    HDC dc = nullptr;
    HBITMAP bitmap = nullptr, oldBitmap = nullptr;
    unsigned char* pixels = nullptr;
    std::string texts[3];
    float floats[14]{};
};

static CFFGLPluginInfo PluginInfo(
    PluginFactory<OrbitText>,
    "ORBT",
    "Orbit Text",
    2, 1,
    1, 0,
    FF_SOURCE,
    "Three independent text lines moving along circular or rounded rectangular paths.",
    "Orbit Text for Resolume Arena"
);
} // namespace


