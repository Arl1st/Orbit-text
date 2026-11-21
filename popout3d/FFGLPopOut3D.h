#pragma once
#include <FFGLSDK.h>

class PopOut3D : public CFFGLPlugin {
public:
 PopOut3D();
 FFResult InitGL(const FFGLViewportStruct* vp) override;
 FFResult ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
 FFResult DeInitGL() override;
 FFResult SetFloatParameter(unsigned int index,float value) override;
 float GetFloatParameter(unsigned int index) override;
private:
 ffglex::FFGLShader shader;
 ffglex::FFGLScreenQuad quad;
 float depth,popOut,focus,cyanMix,glow,direction,blend;
 float foreground,background,popStrength,depthMap,invertDepth;
};
