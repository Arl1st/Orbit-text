#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <FFGLSDK.h>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <chrono>
using namespace ffglex;

namespace {
enum Param : unsigned int {
 T1,E1,S1,W1,H1,R1,V1,D1,O1,P1,F1,L1,CR1,CG1,CB1,GI1,GR1,GE1,
 T2,E2,S2,W2,H2,R2,V2,D2,O2,P2,F2,L2,CR2,CG2,CB2,GI2,GR2,GE2,
 T3,E3,S3,W3,H3,R3,V3,D3,O3,P3,F3,L3,CR3,CG3,CB3,GI3,GR3,GE3
};
struct Line {
 std::string text;
 float enabled=1, shape=1, width=.46f, height=.25f, radius=.12f;
 float speed=.18f, reverse=0, offset=0, pause=0, font=.34f, spacing=.5f;
 float r=1,g=1,b=1; float glow=0.65f, glowRadius=0.012f; bool glowEnabled=true;
};
struct Canvas { HDC dc=nullptr; HBITMAP bmp=nullptr,old=nullptr; unsigned char* px=nullptr; GLuint tex=0; };
struct Point { float x,y,tx,ty; };
const char* vs=R"(#version 410 core
layout(location=0) in vec4 vPosition; layout(location=1) in vec2 vUV;
out vec2 uv; void main(){gl_Position=vPosition;uv=vUV;})";
const char* fs=R"(#version 410 core
uniform sampler2D Mask; uniform vec3 Color; uniform vec2 TexelSize; uniform float GlowAmount; uniform float GlowRadius; in vec2 uv; out vec4 fragColor;
void main(){
 vec2 p=vec2(uv.x,1.0-uv.y); float a=texture(Mask,p).a;
 vec2 d=TexelSize*GlowRadius; float blur=0.0;
 blur+=texture(Mask,p+vec2(d.x,0)).a; blur+=texture(Mask,p-vec2(d.x,0)).a;
 blur+=texture(Mask,p+vec2(0,d.y)).a; blur+=texture(Mask,p-vec2(0,d.y)).a;
 blur+=texture(Mask,p+vec2(d.x,d.y)).a; blur+=texture(Mask,p+vec2(d.x,-d.y)).a;
 blur+=texture(Mask,p+vec2(-d.x,d.y)).a; blur+=texture(Mask,p-vec2(d.x,d.y)).a;
 float glow=clamp((blur/8.0)*GlowAmount,0.0,1.0);
 fragColor=vec4(Color,max(a,glow));
})";
std::wstring Wide(const std::string& s) {
 if(s.empty()) return L"";
 int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),(int)s.size(),nullptr,0);
 if(n<=0) return std::wstring(s.begin(),s.end());
 std::wstring out(n,L'\0'); MultiByteToWideChar(CP_UTF8,0,s.c_str(),(int)s.size(),&out[0],n); return out;
}
Point RectPoint(float d,float cx,float cy,float rx,float ry,float r) {
 r=std::max(0.0f,std::min(r,std::min(rx,ry)));
 const float sw=2*(rx-r), sh=2*(ry-r), arc=1.57079632679f*r;
 const float total=2*sw+2*sh+4*arc;
 if(total<1) return {cx,cy,1,0};
 d=std::fmod(d,total); if(d<0)d+=total;
 if(d<sw)return {cx-rx+r+d,cy-ry,1,0}; d-=sw;
 if(d<arc){float a=-1.57079632679f+d/std::max(r,.001f);return {cx+rx-r+r*cosf(a),cy-ry+r+r*sinf(a),-sinf(a),cosf(a)};} d-=arc;
 if(d<sh)return {cx+rx,cy-ry+r+d,0,1}; d-=sh;
 if(d<arc){float a=d/std::max(r,.001f);return {cx+rx-r+r*cosf(a),cy+ry-r+r*sinf(a),-sinf(a),cosf(a)};} d-=arc;
 if(d<sw)return {cx+rx-r-d,cy+ry,-1,0}; d-=sw;
 if(d<arc){float a=1.57079632679f+d/std::max(r,.001f);return {cx-rx+r+r*cosf(a),cy+ry-r+r*sinf(a),-sinf(a),cosf(a)};} d-=arc;
 if(d<sh)return {cx-rx,cy+ry-r-d,0,-1}; d-=sh;
 float a=3.14159265359f+d/std::max(r,.001f);
 return {cx-rx+r+r*cosf(a),cy-ry+r+r*sinf(a),-sinf(a),cosf(a)};
}
class OrbitText final : public CFFGLPlugin {
public:
 OrbitText():CFFGLPlugin(false) {
  SetMinInputs(0);SetMaxInputs(0);
  lines[0].text="ORBIT TEXT";lines[1].text="RESOLUME ARENA";lines[2].text="LIVE VISUALS";
  const unsigned int base[3]={T1,T2,T3}; const char* names[3]={"Line 1","Line 2","Line 3"};
  for(int i=0;i<3;++i){
   unsigned int b=base[i];std::string n=names[i];
   SetParamInfo(b,n.append(" Text").c_str(),FF_TYPE_TEXT,lines[i].text.c_str()); n=names[i];
   SetParamInfo(b+1,(n+" Visible").c_str(),FF_TYPE_BOOLEAN,true);
   SetOptionParamInfo(b+2,(n+" Path").c_str(),2,1.0f);
   SetParamElementInfo(b+2,0,"Circle",0);SetParamElementInfo(b+2,1,"Rounded Rectangle",1);
   SetParamInfof(b+3,(n+" Width").c_str(),FF_TYPE_STANDARD);
   SetParamInfof(b+4,(n+" Height").c_str(),FF_TYPE_STANDARD);
   SetParamInfof(b+5,(n+" Corner Radius").c_str(),FF_TYPE_STANDARD);
   SetParamInfof(b+6,(n+" Speed").c_str(),FF_TYPE_STANDARD);
   SetParamInfo(b+7,(n+" Reverse").c_str(),FF_TYPE_BOOLEAN,false);
   SetParamInfof(b+8,(n+" Start Offset").c_str(),FF_TYPE_STANDARD);
   SetParamInfo(b+9,(n+" Pause").c_str(),FF_TYPE_BOOLEAN,false);
   SetParamInfof(b+10,(n+" Font Size").c_str(),FF_TYPE_STANDARD);
   SetParamInfof(b+11,(n+" Letter Spacing").c_str(),FF_TYPE_STANDARD);
   SetParamInfof(b+12,(n+" Red").c_str(),FF_TYPE_RED);
   SetParamInfof(b+13,(n+" Green").c_str(),FF_TYPE_GREEN);
   SetParamInfof(b+14,(n+" Blue").c_str(),FF_TYPE_BLUE);
   SetParamInfof(b+15,(n+" Glow Intensity").c_str(),FF_TYPE_STANDARD);
   SetParamInfof(b+16,(n+" Glow Radius").c_str(),FF_TYPE_STANDARD);
   SetParamInfo(b+17,(n+" Glow Enabled").c_str(),FF_TYPE_BOOLEAN,true);
  }
  SetTimeSupported(true);
 }
 FFResult InitGL(const FFGLViewportStruct* vp) override {
  if(!shader.Compile(vs,fs)||!quad.Initialise()){DeInitGL();return FF_FAIL;}
  ScopedShaderBinding bind(shader.GetGLID());maskLoc=shader.FindUniform("Mask");colorLoc=shader.FindUniform("Color");texelLoc=shader.FindUniform("TexelSize");glowLoc=shader.FindUniform("GlowAmount");glowRadiusLoc=shader.FindUniform("GlowRadius");
  for(auto& c:canvases)if(!CreateCanvas(c,vp->width,vp->height)){DeInitGL();return FF_FAIL;}
  return CFFGLPlugin::InitGL(vp);
 }
 FFResult ProcessOpenGL(ProcessOpenGLStruct*) override {
  if(width<1||height<1)return FF_FAIL;
  static const auto start=std::chrono::steady_clock::now();
  float now=std::chrono::duration<float>(std::chrono::steady_clock::now()-start).count();
  for(int i=0;i<3;++i){
   if(!lines[i].enabled)continue;
   DrawLine(i,now);Canvas& c=canvases[i];
   glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,c.tex);
   glPixelStorei(GL_UNPACK_ALIGNMENT,4);
   glTexSubImage2D(GL_TEXTURE_2D,0,0,0,width,height,GL_BGRA,GL_UNSIGNED_BYTE,c.px);
   ScopedShaderBinding bind(shader.GetGLID());
   glUniform1i(maskLoc,0);glUniform3f(colorLoc,lines[i].r,lines[i].g,lines[i].b);
   glUniform2f(texelLoc,1.0f/width,1.0f/height);glUniform1f(glowLoc,lines[i].glowEnabled?lines[i].glow:0.0f);glUniform1f(glowRadiusLoc,lines[i].glowRadius*std::min(width,height));
   glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   quad.Draw();glBlendFunc(GL_ONE,GL_ZERO);glDisable(GL_BLEND);
  }
  return FF_SUCCESS;
 }
 FFResult DeInitGL() override {
  for(auto& c:canvases)DestroyCanvas(c);
  shader.FreeGLResources();quad.Release();return FF_SUCCESS;
 }
 unsigned int Resize(const FFGLViewportStruct* vp) override {
  CFFGLPlugin::Resize(vp);for(auto& c:canvases)if(!CreateCanvas(c,vp->width,vp->height))return FF_FAIL;
  return FF_SUCCESS;
 }
 FFResult SetTextParameter(unsigned int index,const char* value) override {
  const unsigned int b[3]={T1,T2,T3};for(int i=0;i<3;++i)if(index==b[i]){lines[i].text=value?value:"";return FF_SUCCESS;}return FF_FAIL;
 }
 char* GetTextParameter(unsigned int index) override {
  const unsigned int b[3]={T1,T2,T3};for(int i=0;i<3;++i)if(index==b[i])return const_cast<char*>(lines[i].text.c_str());return nullptr;
 }
 FFResult SetFloatParameter(unsigned int index,float v) override {
  const unsigned int b[3]={T1,T2,T3};
  for(int i=0;i<3;++i){if(index<b[i]||index>b[i]+17)continue;Line& x=lines[i];
   switch(index-b[i]){
    case 1:x.enabled=v>.5f;break;case 2:x.shape=v;break;case 3:x.width=.05f+v*.85f;break;
    case 4:x.height=.05f+v*.85f;break;case 5:x.radius=v*.5f;break;case 6:x.speed=v*.5f;break;
    case 7:x.reverse=v>.5f;break;case 8:x.offset=v;break;case 9:x.pause=v>.5f;break;
    case 10:x.font=.08f+v*.72f;break;case 11:x.spacing=.5f+v*2.5f;break;
    case 12:x.r=v;break;case 13:x.g=v;break;case 14:x.b=v;break;case 15:x.glow=v*3.0f;break;case 16:x.glowRadius=.002f+v*.045f;break;case 17:x.glowEnabled=v>.5f;break;
   }return FF_SUCCESS;
  }return FF_FAIL;
 }
 float GetFloatParameter(unsigned int index) override {
  const unsigned int b[3]={T1,T2,T3};
  for(int i=0;i<3;++i){if(index<b[i]||index>b[i]+17)continue;const Line& x=lines[i];
   switch(index-b[i]){
    case 1:return x.enabled;case 2:return x.shape;case 3:return (x.width-.05f)/.85f;case 4:return (x.height-.05f)/.85f;
    case 5:return x.radius/.5f;case 6:return x.speed/.5f;case 7:return x.reverse;case 8:return x.offset;
    case 9:return x.pause;case 10:return (x.font-.08f)/.72f;case 11:return (x.spacing-.5f)/2.5f;
    case 12:return x.r;case 13:return x.g;case 14:return x.b;case 15:return x.glow/3.0f;case 16:return (x.glowRadius-.002f)/.045f;case 17:return x.glowEnabled;
   }
  }return 0;
 }
private:
 bool CreateCanvas(Canvas& c,int w,int h){
  DestroyCanvas(c);width=std::max(64,std::min(w,4096));height=std::max(64,std::min(h,4096));
  BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;
  bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
  c.dc=CreateCompatibleDC(nullptr);if(!c.dc)return false;
  c.bmp=CreateDIBSection(c.dc,&bi,DIB_RGB_COLORS,(void**)&c.px,nullptr,0);
  if(!c.bmp||!c.px){DestroyCanvas(c);return false;}
  c.old=(HBITMAP)SelectObject(c.dc,c.bmp);SetGraphicsMode(c.dc,GM_ADVANCED);
  glGenTextures(1,&c.tex);glBindTexture(GL_TEXTURE_2D,c.tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_BGRA,GL_UNSIGNED_BYTE,c.px);
  return true;
 }
 void DestroyCanvas(Canvas& c){
  if(c.tex){glDeleteTextures(1,&c.tex);c.tex=0;}
  if(c.dc&&c.old)SelectObject(c.dc,c.old);if(c.bmp)DeleteObject(c.bmp);if(c.dc)DeleteDC(c.dc);
  c.dc=nullptr;c.bmp=nullptr;c.old=nullptr;c.px=nullptr;
 }
 void DrawLine(int i,float now){
  Canvas& c=canvases[i];Line& x=lines[i];std::memset(c.px,0,(size_t)width*height*4);
  std::wstring text=Wide(x.text);if(text.empty())return;
  int fs=std::max(8,(int)(10+x.font*110));HFONT font=CreateFontW(-fs,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
    OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Arial");
  if(!font)return;HGDIOBJ old=SelectObject(c.dc,font);SetBkMode(c.dc,TRANSPARENT);SetTextColor(c.dc,RGB(255,255,255));
  std::vector<int> adv(text.size());SIZE sz{};float total=0;
  for(size_t k=0;k<text.size();++k){GetTextExtentPoint32W(c.dc,&text[k],1,&sz);adv[k]=std::max(1L,sz.cx+(int)(x.spacing*3));total+=adv[k];}
  float cx=width*.5f,cy=height*.5f,rx=std::max(24.0f,x.width*width*.5f),ry=std::max(24.0f,x.height*height*.5f);
  bool circle=x.shape<.5f;if(circle){float rad=std::min(rx,ry);rx=rad;ry=rad;}
  float r=circle?std::min(rx,ry):std::min(x.radius*std::min(width,height),std::min(rx,ry));
  float sw=2*(rx-r),sh=2*(ry-r),arc=1.57079632679f*r;
  float perimeter=circle?6.2831853f*std::sqrt((rx*rx+ry*ry)*.5f):2*sw+2*sh+4*arc;
  float direction=x.reverse?-1.0f:1.0f;
  float startOffset=x.offset*perimeter;
  float movement=x.pause?0.0f:now*x.speed*std::min(width,height)*direction;
  float cursor=-total*.5f+startOffset+movement;
  for(size_t k=0;k<text.size();++k){
   float d=cursor+adv[k]*.5f;Point p{};
   if(circle){float a=d/perimeter*6.2831853f;p={cx+rx*cosf(a),cy+ry*sinf(a),-sinf(a),cosf(a)};}
   else p=RectPoint(d,cx,cy,rx,ry,r);
   XFORM xf{};float nx=-p.ty,ny=p.tx;xf.eM11=p.tx;xf.eM12=p.ty;xf.eM21=nx;xf.eM22=ny;
   xf.eDx=p.x-nx*fs*.55f;xf.eDy=p.y-ny*fs*.55f;SetWorldTransform(c.dc,&xf);
   TextOutW(c.dc,0,0,&text[k],1);cursor+=adv[k];
  }
  ModifyWorldTransform(c.dc,nullptr,MWT_IDENTITY);SelectObject(c.dc,old);DeleteObject(font);
  for(size_t k=0,n=(size_t)width*height;k<n;++k){unsigned char* p=c.px+k*4;unsigned char a=std::max(p[0],std::max(p[1],p[2]));p[0]=p[1]=p[2]=255;p[3]=a;}
 }
 std::array<Line,3> lines{};std::array<Canvas,3> canvases{};
 FFGLShader shader;FFGLScreenQuad quad;GLint maskLoc=-1,colorLoc=-1,texelLoc=-1,glowLoc=-1,glowRadiusLoc=-1;int width=0,height=0;
};
static CFFGLPluginInfo PluginInfo(PluginFactory<OrbitText>,"ORBT","Orbit Text",2,1,1,0,FF_SOURCE,
 "Three independently configurable text lines on editable orbit paths","Orbit Text for Resolume Arena");
}
