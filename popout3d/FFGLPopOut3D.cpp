#include "FFGLPopOut3D.h"
using namespace ffglex;
enum Param : unsigned int { P_DEPTH, P_POPOUT, P_FOCUS, P_CYAN, P_GLOW, P_DIRECTION, P_BLEND };
static CFFGLPluginInfo PluginInfo(PluginFactory< PopOut3D >,"P3D1","POP-OUT 3D",2,1,1,0,FF_EFFECT,"Anaglyph stereoscopic depth and pop-out effect","POP-OUT 3D for Resolume Arena");
static const char VertexShader[] = R"(#version 410 core
uniform vec2 MaxUV; layout(location=0) in vec4 vPosition; layout(location=1) in vec2 vUV; out vec2 uv;
void main(){ gl_Position=vPosition; uv=vUV*MaxUV; })";
static const char FragmentShader[] = R"(#version 410 core
uniform sampler2D InputTexture; uniform float Depth; uniform float PopOut; uniform float Focus; uniform float CyanMix; uniform float Glow; uniform float Direction; uniform float Blend;
in vec2 uv; out vec4 fragColor;
float lum(vec3 c){ return dot(c,vec3(0.2126,0.7152,0.0722)); }
void main(){
 vec4 base=texture(InputTexture,uv);
 if(base.a<=0.0001){ fragColor=base; return; }
 vec3 src=base.rgb/base.a;

 // Single-image pseudo depth: bright areas move toward the viewer,
 // darker areas recede. The focus control sets the neutral depth plane.
 float centered=clamp((lum(src)-Focus)*2.0,-1.0,1.0);
 float z=sign(centered)*pow(abs(centered),0.82);
 float foreground=max(z,0.0);
 float parallax=(Depth*0.020*z + PopOut*0.035*foreground)*Direction;

 // Red is the left-eye view; green/blue form the cyan right-eye view.
 vec2 shift=vec2(parallax,0.0);
 vec2 uvL=clamp(uv+shift,vec2(0.001),vec2(0.999));
 vec2 uvR=clamp(uv-shift,vec2(0.001),vec2(0.999));
 vec4 l=texture(InputTexture,uvL);
 vec4 r=texture(InputTexture,uvR);
 vec3 left=l.rgb/max(l.a,0.0001);
 vec3 right=r.rgb/max(r.a,0.0001);
 vec3 anaglyph=vec3(left.r,mix(src.g,right.g,CyanMix),mix(src.b,right.b,CyanMix));

 // Fine edge accent; kept deliberately subtle so it does not replace depth.
 vec2 px=1.0/vec2(textureSize(InputTexture,0));
 float edge=abs(lum(texture(InputTexture,uv+vec2(px.x,0)).rgb)-lum(texture(InputTexture,uv-vec2(px.x,0)).rgb))
           +abs(lum(texture(InputTexture,uv+vec2(0,px.y)).rgb)-lum(texture(InputTexture,uv-vec2(0,px.y)).rgb));
 vec3 outColor=mix(src,anaglyph,Blend)+vec3(1.0,0.18,0.08)*edge*Glow*1.25;
 fragColor=vec4(clamp(outColor,0.0,1.0)*base.a,base.a);
})";
PopOut3D::PopOut3D():depth(0.65f),popOut(0.7f),focus(0.5f),cyanMix(1.0f),glow(0.12f),direction(1.0f),blend(1.0f){
 SetMinInputs(1);SetMaxInputs(1);
 SetParamInfof(P_DEPTH,"3D Depth",FF_TYPE_STANDARD);SetParamInfof(P_POPOUT,"Pop Out",FF_TYPE_STANDARD);
 SetParamInfof(P_FOCUS,"Depth Focus",FF_TYPE_STANDARD);SetParamInfof(P_CYAN,"Cyan Channel",FF_TYPE_STANDARD);
 SetParamInfof(P_GLOW,"Edge Glow",FF_TYPE_STANDARD);SetParamInfof(P_DIRECTION,"3D Direction",FF_TYPE_STANDARD);
 SetParamInfof(P_BLEND,"3D Blend",FF_TYPE_STANDARD);
}
FFResult PopOut3D::InitGL(const FFGLViewportStruct* vp){if(!shader.Compile(VertexShader,FragmentShader)||!quad.Initialise()){DeInitGL();return FF_FAIL;}return CFFGLPlugin::InitGL(vp);}
FFResult PopOut3D::ProcessOpenGL(ProcessOpenGLStruct* pGL){
 if(!pGL||pGL->numInputTextures<1||!pGL->inputTextures[0])return FF_FAIL;
 ScopedShaderBinding shaderBinding(shader.GetGLID());ScopedSamplerActivation activateSampler(0);Scoped2DTextureBinding textureBinding(pGL->inputTextures[0]->Handle);
 shader.Set("InputTexture",0);FFGLTexCoords maxCoords=GetMaxGLTexCoords(*pGL->inputTextures[0]);shader.Set("MaxUV",maxCoords.s,maxCoords.t);
 shader.Set("Depth",depth);shader.Set("PopOut",popOut);shader.Set("Focus",focus);shader.Set("CyanMix",cyanMix);
 shader.Set("Glow",glow);shader.Set("Direction",direction<0.5f?-1.0f:1.0f);shader.Set("Blend",blend);quad.Draw();return FF_SUCCESS;
}
FFResult PopOut3D::DeInitGL(){shader.FreeGLResources();quad.Release();return FF_SUCCESS;}
FFResult PopOut3D::SetFloatParameter(unsigned int i,float v){switch(i){case P_DEPTH:depth=v;break;case P_POPOUT:popOut=v;break;case P_FOCUS:focus=v;break;case P_CYAN:cyanMix=v;break;case P_GLOW:glow=v;break;case P_DIRECTION:direction=v;break;case P_BLEND:blend=v;break;default:return FF_FAIL;}return FF_SUCCESS;}
float PopOut3D::GetFloatParameter(unsigned int i){switch(i){case P_DEPTH:return depth;case P_POPOUT:return popOut;case P_FOCUS:return focus;case P_CYAN:return cyanMix;case P_GLOW:return glow;case P_DIRECTION:return direction;case P_BLEND:return blend;}return 0.0f;}
