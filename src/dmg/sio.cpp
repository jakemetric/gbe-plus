// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.cpp
// Date : April 30, 2016
// Description : Game Boy Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates Gameboy-to-Gameboy data transfers

#include "sio.h"

/****** SIO Constructor ******/
DMG_SIO::DMG_SIO()
{
	reset();
}

/****** SIO Destructor ******/
DMG_SIO::~DMG_SIO()
{
	std::cout<<"SIO::Shutdown\n";
}

/****** Reset SIO ******/
void DMG_SIO::reset()
{
	sio_stat.double_speed = false;
	sio_stat.internal_clock = false;
	sio_stat.connected = false;
	sio_stat.shifts_left = 0;
	sio_stat.shift_counter = 0;
}

/****** Tranfers one bit to another system ******/
bool DMG_SIO::send_bit() { return true; }

/****** Receives one bit from another system ******/
bool DMG_SIO::receive_bit() { return true; }