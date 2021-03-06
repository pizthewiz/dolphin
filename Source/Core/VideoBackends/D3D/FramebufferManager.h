// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#ifdef HAVE_OCULUSSDK
#define OVR_D3D_VERSION 11

#include "Kernel/OVR_Types.h"
#include "OVR_CAPI.h"
#include "OVR_CAPI_D3D.h"
#include "Kernel/OVR_Math.h"

extern "C"
{
	void ovrhmd_EnableHSWDisplaySDKRender(ovrHmd hmd, ovrBool enabled);
}
#endif

#include "d3d11.h"

#include "VideoBackends/D3D/D3DTexture.h"
#include "VideoCommon/FramebufferManagerBase.h"

namespace DX11 {

// On the GameCube, the game sends a request for the graphics processor to
// transfer its internal EFB (Embedded Framebuffer) to an area in GameCube RAM
// called the XFB (External Framebuffer). The size and location of the XFB is
// decided at the time of the copy, and the format is always YUYV. The video
// interface is given a pointer to the XFB, which will be decoded and
// displayed on the TV.
//
// There are two ways for Dolphin to emulate this:
//
// Real XFB mode:
//
// Dolphin will behave like the GameCube and encode the EFB to
// a portion of GameCube RAM. The emulated video interface will decode the data
// for output to the screen.
//
// Advantages: Behaves exactly like the GameCube.
// Disadvantages: Resolution will be limited.
//
// Virtual XFB mode:
//
// When a request is made to copy the EFB to an XFB, Dolphin
// will remember the RAM location and size of the XFB in a Virtual XFB list.
// The video interface will look up the XFB in the list and use the enhanced
// data stored there, if available.
//
// Advantages: Enables high resolution graphics, better than real hardware.
// Disadvantages: If the GameCube CPU writes directly to the XFB (which is
// possible but uncommon), the Virtual XFB will not capture this information.

// There may be multiple XFBs in GameCube RAM. This is the maximum number to
// virtualize.

struct XFBSource : public XFBSourceBase
{
	XFBSource(D3DTexture2D *_tex) : tex(_tex) {}
	~XFBSource() { tex->Release(); }

	void Draw(const MathUtil::Rectangle<int> &sourcerc,
		const MathUtil::Rectangle<float> &drawrc) const override;
	void DecodeToTexture(u32 xfbAddr, u32 fbWidth, u32 fbHeight) override;
	void CopyEFB(float Gamma) override;

	D3DTexture2D* const tex;
};

class FramebufferManager : public FramebufferManagerBase
{
public:
	FramebufferManager();
	~FramebufferManager();

	static D3DTexture2D* &GetEFBColorTexture(int eye);
	static ID3D11Texture2D* &GetEFBColorStagingBuffer(int eye);

	static D3DTexture2D* &GetEFBDepthTexture(int eye);
	static D3DTexture2D* &GetEFBDepthReadTexture(int eye);
	static ID3D11Texture2D* &GetEFBDepthStagingBuffer(int eye);

	static D3DTexture2D* &GetResolvedEFBColorTexture(int eye);
	static D3DTexture2D* &GetResolvedEFBDepthTexture(int eye);

	static D3DTexture2D* &GetEFBColorTempTexture(int eye) { return m_efb[eye].color_temp_tex; }
	static void SwapReinterpretTexture(int eye)
	{
		D3DTexture2D* swaptex = GetEFBColorTempTexture(eye);
		m_efb[eye].color_temp_tex = GetEFBColorTexture(eye);
		m_efb[eye].color_tex = swaptex;
	}

	static void RenderToEye(int eye);
	static void SwapRenderEye();

	static void SwapAsyncFrontBuffers();

	// Oculus Rift
#ifdef HAVE_OCULUSSDK
	static void ConfigureRift();
	static ovrD3D11Texture m_eye_texture[2];
#endif
	//static volatile GLuint m_frontBuffer[2];
	static bool m_stereo3d;
	static int m_eye_count, m_current_eye;

private:
	XFBSourceBase* CreateXFBSource(unsigned int target_width, unsigned int target_height) override;
	void GetTargetSize(unsigned int *width, unsigned int *height, const EFBRectangle& sourceRc) override;

	void CopyToRealXFB(u32 xfbAddr, u32 fbWidth, u32 fbHeight, const EFBRectangle& sourceRc,float Gamma) override;

	static struct Efb
	{
		D3DTexture2D* color_tex;
		ID3D11Texture2D* color_staging_buf;

		D3DTexture2D* depth_tex;
		ID3D11Texture2D* depth_staging_buf;
		D3DTexture2D* depth_read_texture;

		D3DTexture2D* color_temp_tex;

		D3DTexture2D* resolved_color_tex;
		D3DTexture2D* resolved_depth_tex;
	} m_efb[2];
};

}  // namespace DX11
