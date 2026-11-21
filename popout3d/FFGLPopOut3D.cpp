#include "FFGLPopOut3D.h"
using namespace ffglex;

enum Param : unsigned int {
 P_DEPTH, P_POPOUT, P_FOCUS, P_CYAN, P_GLOW, P_DIRECTION, P_BLEND,
 P_FOREGROUND, P_BACKGROUND, P_STRENGTH, P_DEPTHMAP, P_INVERT
};

static CFFGLPluginInfo PluginInfo(
 PluginFactory< PopOut3D >,"P3D1","POP-OUT 3D",3,0,0,0,FF_EFFECT,
 "Anaglyph stereoscopic depth and pop-out effect","POP-OUT 3D for Resolume Arena");

static const char VertexShader[] = R"(#version 410 core
uniform vec2 MaxUV;
layout(location=0) in vec4 vPosition;
layout(location=1) in vec2 vUV;
out vec2 uv;
void main(){ gl_Position=vPosition; uv=vUV*MaxUV; }
)";

static const char FragmentShader[] = R"(#version 410 core
uniform sampler2D InputTexture;
uniform float Depth;
uniform float PopOut;
uniform float Focus;
uniform float CyanMix;
uniform float Glow;
uniform float Direction;
uniform float Blend;
uniform float Foreground;
uniform float Background;
uniform float PopStrength;
uniform float DepthMap;
uniform float InvertDepth;

in vec2 uv;
out vec4 fragColor;

float luminance(vec3 c){ return dot(c,vec3(0.2126,0.7152,0.0722)); }

void main(){
 vec4 base=texture(InputTexture,uv);
 if(base.a<=0.0001){ fragColor=base; return; }
 vec3 src=base.rgb/max(base.a,0.0001);

 // Adjustable pseudo-depth map derived from source luminance.
 // DepthMap=0 gives a neutral plane; DepthMap=1 uses full luminance contrast.
 float luma=luminance(src);
 float mapValue=mix(0.5,luma,DepthMap);
 float centered=clamp((mapValue-Focus)*2.0,-1.0,1.0);
 if(InvertDepth>=0.5) centered=-centered;
 float z=sign(centered)*pow(abs(centered),0.85);
 float nearSide=max(z,0.0);
 float farSide=max(-z,0.0);

 // Independent foreground/background disparity plus extra pop-out.
 float disparity=(nearSide*Foreground - farSide*Background
                 + nearSide*PopOut*PopStrength) * Depth * 0.035 * Direction;

 vec2 shift=vec2(disparity,0.0);
 vec2 uvL=clamp(uv+shift,vec2(0.001),vec2(0.999));
 vec2 uvR=clamp(uv-shift,vec2(0.001),vec2(0.999));
 vec4 leftSample=texture(InputTexture,uvL);
 vec4 rightSample=texture(InputTexture,uvR);
 vec3 leftColor=leftSample.rgb/max(leftSample.a,0.0001);
 vec3 rightColor=rightSample.rgb/max(rightSample.a,0.0001);

 // Classic red/cyan anaglyph: red from left eye, green/blue from right eye.
 vec3 anaglyph=vec3(leftColor.r,
                   mix(src.g,rightColor.g,CyanMix),
                   mix(src.b,rightColor.b,CyanMix));

 vec2 px=1.0/vec2(textureSize(InputTexture,0));
 float edge=abs(luminance(texture(InputTexture,uv+vec2(px.x,0)).rgb)-
                luminance(texture(InputTexture,uv-vec2(px.x,0)).rgb))
           +abs(luminance(texture(InputTexture,uv+vec2(0,px.y)).rgb)-
                luminance(texture(InputTexture,uv-vec2(0,px.y)).rgb));
 vec3 result=mix(src,anaglyph,Blend)
             +vec3(1.0,0.18,0.08)*edge*Glow*1.1;
 fragColor=vec4(clamp(result,0.0,1.0)*base.a,base.a);
}
)";

PopOut3D::PopOut3D()
 : depth(0.85f),popOut(0.75f),focus(0.5f),cyanMix(1.0f),
   glow(0.10f),direction(1.0f),blend(1.0f),
   foreground(1.0f),background(0.45f),popStrength(1.0f),
   depthMap(1.0f),invertDepth(0.0f)
{
 SetMinInputs(1); SetMaxInputs(1);
 SetParamInfof(P_DEPTH,"3D Depth",FF_TYPE_STANDARD);
 SetParamInfof(P_POPOUT,"Pop Out",FF_TYPE_STANDARD);
 SetParamInfof(P_FOCUS,"Depth Focus",FF_TYPE_STANDARD);
 SetParamInfof(P_CYAN,"Cyan Channel",FF_TYPE_STANDARD);
 SetParamInfof(P_GLOW,"Edge Glow",FF_TYPE_STANDARD);
 SetParamInfof(P_DIRECTION,"3D Direction",FF_TYPE_STANDARD);
 SetParamInfof(P_BLEND,"3D Blend",FF_TYPE_STANDARD);
 SetParamInfof(P_FOREGROUND,"Foreground Depth",FF_TYPE_STANDARD);
 SetParamInfof(P_BACKGROUND,"Background Depth",FF_TYPE_STANDARD);
 SetParamInfof(P_STRENGTH,"Pop Out Strength",FF_TYPE_STANDARD);
 SetParamInfof(P_DEPTHMAP,"Depth Map",FF_TYPE_STANDARD);
 SetParamInfof(P_INVERT,"Invert Depth",FF_TYPE_STANDARD);
}

FFResult PopOut3D::InitGL(const FFGLViewportStruct* vp){
 if(!shader.Compile(VertexShader,FragmentShader)||!quad.Initialise()){
  DeInitGL(); return FF_FAIL;
 }
 return CFFGLPlugin::InitGL(vp);
}

FFResult PopOut3D::ProcessOpenGL(ProcessOpenGLStruct* pGL){
 if(!pGL||pGL->numInputTextures<1||!pGL->inputTextures[0]) return FF_FAIL;
 ScopedShaderBinding shaderBinding(shader.GetGLID());
 ScopedSamplerActivation activateSampler(0);
 Scoped2DTextureBinding textureBinding(pGL->inputTextures[0]->Handle);
 shader.Set("InputTexture",0);
 FFGLTexCoords maxCoords=GetMaxGLTexCoords(*pGL->inputTextures[0]);
 shader.Set("MaxUV",maxCoords.s,maxCoords.t);
 shader.Set("Depth",depth); shader.Set("PopOut",popOut);
 shader.Set("Focus",focus); shader.Set("CyanMix",cyanMix);
 shader.Set("Glow",glow); shader.Set("Direction",direction<0.5f?-1.0f:1.0f);
 shader.Set("Blend",blend); shader.Set("Foreground",foreground);
 shader.Set("Background",background); shader.Set("PopStrength",popStrength);
 shader.Set("DepthMap",depthMap); shader.Set("InvertDepth",invertDepth);
 quad.Draw();
 return FF_SUCCESS;
}

FFResult PopOut3D::DeInitGL(){
 shader.FreeGLResources(); quad.Release(); return FF_SUCCESS;
}

FFResult PopOut3D::SetFloatParameter(unsigned int i,float v){
 switch(i){
  case P_DEPTH:depth=v;break; case P_POPOUT:popOut=v;break;
  case P_FOCUS:focus=v;break; case P_CYAN:cyanMix=v;break;
  case P_GLOW:glow=v;break; case P_DIRECTION:direction=v;break;
  case P_BLEND:blend=v;break; case P_FOREGROUND:foreground=v;break;
  case P_BACKGROUND:background=v;break; case P_STRENGTH:popStrength=v;break;
  case P_DEPTHMAP:depthMap=v;break; case P_INVERT:invertDepth=v;break;
  default:return FF_FAIL;
 }
 return FF_SUCCESS;
}

float PopOut3D::GetFloatParameter(unsigned int i){
 switch(i){
  case P_DEPTH:return depth; case P_POPOUT:return popOut;
  case P_FOCUS:return focus; case P_CYAN:return cyanMix;
  case P_GLOW:return glow; case P_DIRECTION:return direction;
  case P_BLEND:return blend; case P_FOREGROUND:return foreground;
  case P_BACKGROUND:return background; case P_STRENGTH:return popStrength;
  case P_DEPTHMAP:return depthMap; case P_INVERT:return invertDepth;
 }
 return 0.0f;
}
