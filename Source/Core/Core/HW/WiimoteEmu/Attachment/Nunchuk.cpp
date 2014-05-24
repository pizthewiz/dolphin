// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "Core/HW/WiimoteEmu/Attachment/Nunchuk.h"

#include "InputCommon/UDPWiimote.h"
#include "InputCommon/UDPWrapper.h"
#ifdef _WIN32
#include "InputCommon/ControllerInterface/Sixense/SixenseHack.h"
#endif

namespace WiimoteEmu
{

static const u8 nunchuck_id[] = { 0x00, 0x00, 0xa4, 0x20, 0x00, 0x00 };
/* Default calibration for the nunchuck. It should be written to 0x20 - 0x3f of the
   extension register. 0x80 is the neutral x and y accelerators and 0xb3 is the
   neutral z accelerometer that is adjusted for gravity. */
static const u8 nunchuck_calibration[] =
{
	0x80, 0x80, 0x80, 0x00, // accelerometer x, y, z neutral
	0xb3, 0xb3, 0xb3, 0x00, //  x, y, z g-force values

	// 0x80 = analog stick x and y axis center
	0xff, 0x00, 0x80,
	0xff, 0x00, 0x80,
	0xec, 0x41 // checksum on the last two bytes
};

static const u8 nunchuk_button_bitmasks[] =
{
	Nunchuk::BUTTON_C,
	Nunchuk::BUTTON_Z,
};

Nunchuk::Nunchuk(UDPWrapper *wrp, WiimoteEmu::ExtensionReg& _reg)
	: Attachment(_trans("Nunchuk"), _reg) , m_udpWrap(wrp)
{
	// buttons
	groups.emplace_back(m_buttons = new Buttons("Buttons"));
	m_buttons->controls.emplace_back(new ControlGroup::Input("C"));
	m_buttons->controls.emplace_back(new ControlGroup::Input("Z"));

	// stick
	groups.emplace_back(m_stick = new AnalogStick("Stick"));

	// swing
	groups.emplace_back(m_swing = new Force("Swing"));

	// tilt
	groups.emplace_back(m_tilt = new Tilt("Tilt"));

	// shake
	groups.emplace_back(m_shake = new Buttons("Shake"));
	m_shake->controls.emplace_back(new ControlGroup::Input("X"));
	m_shake->controls.emplace_back(new ControlGroup::Input("Y"));
	m_shake->controls.emplace_back(new ControlGroup::Input("Z"));

	// set up register
	// calibration
	memcpy(&calibration, nunchuck_calibration, sizeof(nunchuck_calibration));
	// id
	memcpy(&id, nunchuck_id, sizeof(nunchuck_id));

	// this should get set to 0 on disconnect, but it isn't, o well
	memset(m_shake_step, 0, sizeof(m_shake_step));
}

void Nunchuk::GetState(u8* const data, const bool focus)
{
	wm_extension* const ncdata = (wm_extension*)data;
	ncdata->bt = 0;

	// stick
	ControlState state[2];
	m_stick->GetState(&state[0], &state[1], 0, 1);

	nu_cal &cal = *(nu_cal*)&reg.calibration;
	nu_js cal_js[2];
	cal_js[0] = *&cal.jx;
	cal_js[1] = *&cal.jy;

	for (int i = 0; i < 2; i++) {
		ControlState &s = *&state[i];
		nu_js c = *&cal_js[i];
		if (s < 0)
			s = s * abs(c.min - c.center) + c.center;
		else if (s > 0)
			s = s * abs(c.max - c.center) + c.center;
		else
			s = c.center;
	}

	ncdata->jx = u8(trim(state[0]));
	ncdata->jy = u8(trim(state[1]));

	if (ncdata->jx != cal.jx.center || ncdata->jy != cal.jy.center)
	{
		if (ncdata->jy == cal.jy.center)
			ncdata->jy = cal.jy.center + 1;
		if (ncdata->jx == cal.jx.center)
			ncdata->jx = cal.jx.center + 1;
	}

	if (!focus)
	{
		ncdata->jx = cal.jx.center;
		ncdata->jy = cal.jy.center;
	}

	AccelData accel;

	// tilt
	EmulateTilt(&accel, m_tilt, focus);

	if (focus)
	{
		// swing
		EmulateSwing(&accel, m_swing);
		// shake
		EmulateShake(&accel, m_shake, m_shake_step);
		// buttons
		m_buttons->GetState(&ncdata->bt, nunchuk_button_bitmasks);
	}

	// flip the button bits :/
	ncdata->bt ^= 0x03;

	if (m_udpWrap->inst)
	{
		if (m_udpWrap->updNun)
		{
			u8 mask;
			float x, y;
			m_udpWrap->inst->getNunchuck(&x, &y, &mask);
			// buttons
			if (mask & UDPWM_NC)
				ncdata->bt &= ~WiimoteEmu::Nunchuk::BUTTON_C;
			if (mask & UDPWM_NZ)
				ncdata->bt &= ~WiimoteEmu::Nunchuk::BUTTON_Z;
			// stick
			if (ncdata->jx == 0x80 && ncdata->jy == 0x80)
			{
				ncdata->jx = u8(0x80 + x*127);
				ncdata->jy = u8(0x80 + y*127);
			}
		}
		if (m_udpWrap->updNunAccel)
		{
			float x, y, z;
			m_udpWrap->inst->getNunchuckAccel(&x, &y, &z);
			accel.x = x;
			accel.y = y;
			accel.z = z;
		}
	}

#ifdef _WIN32
	// VR Sixense Razer hydra support
	// Left controller will be nunchuck: stick=stick, LB (or 1)=C, LT (or 2)=Z
	if (HydraUpdate() && g_hydra.c[0].enabled)
	{
		const int left = 0, right = 1;
		if ((g_hydra.c[left].buttons & HYDRA_BUTTON_BUMPER) || (g_hydra.c[left].buttons & HYDRA_BUTTON_1))
			ncdata->bt &= ~WiimoteEmu::Nunchuk::BUTTON_C;
		if (g_hydra.c[left].trigger > 0.25f || (g_hydra.c[left].buttons & HYDRA_BUTTON_2))
			ncdata->bt &= ~WiimoteEmu::Nunchuk::BUTTON_Z;
		ncdata->jx = u8(0x80 + g_hydra_state[left].jx * 127);
		ncdata->jy = u8(0x80 + g_hydra_state[left].jy * 127);

		if (!g_hydra.c[left].docked)
		{
			// Note that here X means to the CONTROLLER'S left, Y means to the CONTROLLER'S tail, and Z means to the CONTROLLER'S top! 
			// Tilt sensing.
			accel.x = -g_hydra.c[left].rotation_matrix[0][1];
			accel.z = g_hydra.c[left].rotation_matrix[1][1];
			accel.y = g_hydra.c[left].rotation_matrix[2][1];

			// World-space accelerations need to be converted into accelerations relative to the Wiimote's sensor.
			float rel_acc[3];
			for (int i = 0; i < 3; ++i)
			{
				rel_acc[i] = g_hydra_state[left].a[0] * g_hydra.c[left].rotation_matrix[i][0]
				           + g_hydra_state[left].a[1] * g_hydra.c[left].rotation_matrix[i][1]
				           + g_hydra_state[left].a[2] * g_hydra.c[left].rotation_matrix[i][2];
			}

			// Convert from metres per second per second to G's, and to Wiimote's coordinate system.
			accel.x -= rel_acc[0] / 9.8f;
			accel.z += rel_acc[1] / 9.8f;
			accel.y += rel_acc[2] / 9.8f;
		}
	}
#endif

	FillRawAccelFromGForceData(*(wm_accel*)&ncdata->ax, ncdata->bt, *(accel_cal*)&reg.calibration, accel);
}

void Nunchuk::LoadDefaults(const ControllerInterface& ciface)
{
	// ugly macroooo
	#define set_control(group, num, str)  (group)->controls[num]->control_ref->expression = (str)

	// Stick
	set_control(m_stick, 0, "W"); // up
	set_control(m_stick, 1, "S"); // down
	set_control(m_stick, 2, "A"); // left
	set_control(m_stick, 3, "D"); // right

	// Buttons
#ifdef _WIN32
	set_control(m_buttons, 0, "LCONTROL");  // C
	set_control(m_buttons, 1, "LSHIFT");    // Z
#elif __APPLE__
	set_control(m_buttons, 0, "Left Control"); // C
	set_control(m_buttons, 1, "Left Shift");   // Z
#else
	set_control(m_buttons, 0, "Control_L"); // C
	set_control(m_buttons, 1, "Shift_L");   // Z
#endif
}

}
