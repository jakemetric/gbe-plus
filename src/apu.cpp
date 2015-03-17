// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.h
// Date : March 05, 2015
// Description : Game Boy Advance APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the GBA's 4 sound channels + DMA channels  

#include <cmath>

#include "apu.h"

/****** APU Constructor ******/
APU::APU()
{
	reset();
}

/****** APU Destructor ******/
APU::~APU()
{
	SDL_CloseAudio();
	std::cout<<"APU::Shutdown\n";
}

/****** Reset APU ******/
void APU::reset()
{
	setup = false;

	apu_stat.sound_on = false;
	apu_stat.stereo = false;

	apu_stat.channel_left_volume = 0;
	apu_stat.channel_right_volume = 0;
	apu_stat.dma_left_volume = 0;
	apu_stat.dma_right_volume = 0;

	
	//Reset Channel 1-4 data
	for(int x = 0; x < 4; x++)
	{
		apu_stat.channel[x].raw_frequency = 0;
		apu_stat.channel[x].output_frequency = 0.0;

		apu_stat.channel[x].duration = 5000;
		apu_stat.channel[x].volume = 0;

		apu_stat.channel[x].playing = false;
		apu_stat.channel[x].enable = false;
		apu_stat.channel[x].length_flag = false;

		apu_stat.channel[x].duty_cycle_start = 0;
		apu_stat.channel[x].duty_cycle_end = 4;

		apu_stat.channel[x].envelope_direction = 2;
		apu_stat.channel[x].envelope_step = 0;
		apu_stat.channel[x].envelope_counter = 0;

		apu_stat.channel[x].sweep_direction = 2;
		apu_stat.channel[x].sweep_shift = 0;
		apu_stat.channel[x].sweep_time = 0;
		apu_stat.channel[x].sweep_counter = 0;

		apu_stat.channel[x].frequency_distance = 0;
		apu_stat.channel[x].sample_length = 0;
	}

	apu_stat.waveram_bank = 0;
	apu_stat.waveram_sample = 0;
	apu_stat.waveram_size = 0;

	//Clear waveram data
	for(int x = 0; x < 0x20; x++) { apu_stat.waveram_data[x] = 0; }
}

/****** Initialize APU with SDL ******/
bool APU::init()
{
	//Initialize audio subsystem
	SDL_InitSubSystem(SDL_INIT_AUDIO);

	//Setup the desired audio specifications
    	desired_spec.freq = 44100;
	desired_spec.format = AUDIO_S16SYS;
    	desired_spec.channels = 1;
    	desired_spec.samples = 2048;
    	desired_spec.callback = audio_callback;
    	desired_spec.userdata = this;

    	//Open SDL audio for desired specifications
	if(SDL_OpenAudio(&desired_spec, &obtained_spec) < 0) 
	{ 
		std::cout<<"APU::Failed to open audio\n";
		return false; 
	}
	else if(desired_spec.format != obtained_spec.format) 
	{ 
		std::cout<<"APU::Could not obtain desired audio format\n";
		return false;
	}

	else
	{
		SDL_PauseAudio(0);
		std::cout<<"APU::Initialized\n";
		return true;
	}
}

/******* Generate samples for GBA sound channel 1 ******/
void APU::generate_channel_1_samples(s16* stream, int length)
{
	//Generate samples from the last output of the channel
	if(apu_stat.channel[0].playing)
	{
		int frequency_samples = 44100/apu_stat.channel[0].output_frequency;

		for(int x = 0; x < length; x++, apu_stat.channel[0].sample_length--)
		{

			//Process audio sweep
			if(apu_stat.channel[0].sweep_time >= 1)
			{
				apu_stat.channel[0].sweep_counter++;

				if(apu_stat.channel[0].sweep_counter >= ((44100.0/128) * apu_stat.channel[0].sweep_time))
				{
					int pre_calc = 0;

					//Increase frequency
					if(apu_stat.channel[0].sweep_direction == 0)
					{
						if(apu_stat.channel[0].sweep_shift >= 1) { pre_calc = (apu_stat.channel[0].raw_frequency >> apu_stat.channel[0].sweep_shift); }

						//When frequency is greater than 131KHz, stop sound
						if((apu_stat.channel[0].raw_frequency + pre_calc) >= 0x800) 
						{ 
							apu_stat.channel[0].volume = apu_stat.channel[0].sweep_shift = apu_stat.channel[0].envelope_step = apu_stat.channel[0].sweep_time = 0; 
							apu_stat.channel[0].playing = false; 
						}

						else 
						{ 
							apu_stat.channel[0].raw_frequency += pre_calc;
							apu_stat.channel[0].output_frequency = 131072.0/(2048 - apu_stat.channel[0].raw_frequency);
							mem->memory_map[SND1CNT_X] = (apu_stat.channel[0].raw_frequency & 0xFF);
							mem->memory_map[SND1CNT_X+1] &= ~0x7;
							mem->memory_map[SND1CNT_X+1] |= ((apu_stat.channel[0].raw_frequency >> 8) & 0x7);
						}
					}

						//Decrease frequency
						else if(apu_stat.channel[0].sweep_direction == 1)
						{
							if(apu_stat.channel[0].sweep_shift >= 1) { pre_calc = (apu_stat.channel[0].raw_frequency >> apu_stat.channel[0].sweep_shift); }

							//Only sweep down when result of frequency change is greater than zero
							if((apu_stat.channel[0].raw_frequency - pre_calc) >= 0) 
							{ 
								apu_stat.channel[0].raw_frequency -= pre_calc;
								apu_stat.channel[0].output_frequency = 131072.0/(2048 - apu_stat.channel[0].raw_frequency);
								mem->memory_map[SND1CNT_X] = (apu_stat.channel[0].raw_frequency & 0xFF);
								mem->memory_map[SND1CNT_X+1] &= ~0x7;
								mem->memory_map[SND1CNT_X+1] |= ((apu_stat.channel[0].raw_frequency >> 8) & 0x7);
							}
						}

						apu_stat.channel[0].sweep_counter = 0;
					}
				} 

			//Process audio envelope
			if(apu_stat.channel[0].envelope_step >= 1)
			{
				apu_stat.channel[0].envelope_counter++;

				if(apu_stat.channel[0].envelope_counter >= ((44100.0/64) * apu_stat.channel[0].envelope_step)) 
				{		
					//Decrease volume
					if((apu_stat.channel[0].envelope_direction == 0) && (apu_stat.channel[0].volume >= 1)) { apu_stat.channel[0].volume--; }
				
					//Increase volume
					else if((apu_stat.channel[0].envelope_direction == 1) && (apu_stat.channel[0].volume < 0xF)) { apu_stat.channel[0].volume++; }

					apu_stat.channel[0].envelope_counter = 0;
				}
			}

			//Process audio waveform
			if(apu_stat.channel[0].sample_length > 0)
			{
				apu_stat.channel[0].frequency_distance++;

				//Reset frequency distance
				if(apu_stat.channel[0].frequency_distance >= frequency_samples) { apu_stat.channel[0].frequency_distance = 0; }
		
				//Generate high wave form if duty cycle is on AND volume is not muted
				if((apu_stat.channel[0].frequency_distance >= (frequency_samples/8) * apu_stat.channel[0].duty_cycle_start) 
				&& (apu_stat.channel[0].frequency_distance < (frequency_samples/8) * apu_stat.channel[0].duty_cycle_end)
				&& (apu_stat.channel[0].volume != 0))
				{
					stream[x] = -32768 + (4369 * apu_stat.channel[0].volume);
				}

				//Generate low wave form if duty cycle is off OR volume is muted
				else { stream[x] = -32768; }

			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[0].sample_length == 0) && (!apu_stat.channel[0].length_flag)) { apu_stat.channel[0].sample_length = (apu_stat.channel[0].duration * 44100)/1000; }

			//Or stop sound after duration has been met, reset Sound 1 On Flag
			else if((apu_stat.channel[0].sample_length == 0) && (apu_stat.channel[0].length_flag)) 
			{ 
				stream[x] = -32768; 
				apu_stat.channel[0].sample_length = 0; 
				apu_stat.channel[0].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
	}
}

/******* Generate samples for GBA sound channel 2 ******/
void APU::generate_channel_2_samples(s16* stream, int length)
{
	//Generate samples from the last output of the channel
	if(apu_stat.channel[1].playing)
	{
		int frequency_samples = 44100/apu_stat.channel[1].output_frequency;

		for(int x = 0; x < length; x++, apu_stat.channel[1].sample_length--)
		{
			//Process audio envelope
			if(apu_stat.channel[1].envelope_step >= 1)
			{
				apu_stat.channel[1].envelope_counter++;

				if(apu_stat.channel[1].envelope_counter >= ((44100.0/64) * apu_stat.channel[1].envelope_step)) 
				{		
					//Decrease volume
					if((apu_stat.channel[1].envelope_direction == 0) && (apu_stat.channel[1].volume >= 1)) { apu_stat.channel[1].volume--; }
				
					//Increase volume
					else if((apu_stat.channel[1].envelope_direction == 1) && (apu_stat.channel[1].volume < 0xF)) { apu_stat.channel[1].volume++; }

					apu_stat.channel[1].envelope_counter = 0;
				}
			}

			//Process audio waveform
			if(apu_stat.channel[1].sample_length > 0)
			{
				apu_stat.channel[1].frequency_distance++;

				//Reset frequency distance
				if(apu_stat.channel[1].frequency_distance >= frequency_samples) { apu_stat.channel[1].frequency_distance = 0; }
		
				//Generate high wave form if duty cycle is on AND volume is not muted
				if((apu_stat.channel[1].frequency_distance >= (frequency_samples/8) * apu_stat.channel[1].duty_cycle_start) 
				&& (apu_stat.channel[1].frequency_distance < (frequency_samples/8) * apu_stat.channel[1].duty_cycle_end)
				&& (apu_stat.channel[1].volume != 0))
				{
					stream[x] = -32768 + (4369 * apu_stat.channel[1].volume);
				}

				//Generate low wave form if duty cycle is off OR volume is muted
				else { stream[x] = -32768; }

			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[1].sample_length == 0) && (!apu_stat.channel[1].length_flag)) { apu_stat.channel[1].sample_length = (apu_stat.channel[1].duration * 44100)/1000; }

			//Or stop sound after duration has been met, reset Sound 1 On Flag
			else if((apu_stat.channel[1].sample_length == 0) && (apu_stat.channel[1].length_flag)) 
			{ 
				stream[x] = -32768; 
				apu_stat.channel[1].sample_length = 0; 
				apu_stat.channel[1].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
	}
}

/******* Generate samples for GBA sound channel 3 ******/
void APU::generate_channel_3_samples(s16* stream, int length)
{
	//Generate samples from the last output of the channel
	if((apu_stat.channel[2].playing) && (apu_stat.channel[2].enable))
	{
		//Determine amount of samples per waveform sample
		double wave_step = (44100.0/apu_stat.channel[2].output_frequency) / 32;
		if(wave_step == 0) { return; }

		int frequency_samples = 44100/apu_stat.channel[2].output_frequency;

		for(int x = 0; x < length; x++, apu_stat.channel[2].sample_length--)
		{
			if(apu_stat.channel[2].sample_length > 0)
			{
				apu_stat.channel[2].frequency_distance++;
				
				//Reset frequency distance
				if(apu_stat.channel[2].frequency_distance >= frequency_samples) { apu_stat.channel[2].frequency_distance = 0; }

				//Determine which step in the waveform the current sample corresponds to
				u8 step = int(floor(apu_stat.channel[2].frequency_distance/wave_step)) % 32;

				//Grab wave RAM sample data for even samples
				if(step % 2 == 0)
				{
					step >>= 1;
					apu_stat.waveram_sample = apu_stat.waveram_data[(apu_stat.waveram_bank << 3) + step] >> 4;
	
					//Scale waveform to S16 audio stream
					switch(apu_stat.channel[2].volume)
					{
						case 0x0: stream[x] = -32768; break;
						case 0x1: stream[x] = -32768 + (4369 * apu_stat.waveram_sample); break;
						case 0x2: stream[x] = (-32768 + (4369 * apu_stat.waveram_sample)) * 0.5; break;
						case 0x3: stream[x] = (-32768 + (4369 * apu_stat.waveram_sample)) * 0.25; break;
						case 0x4: stream[x] = (-32768 + (4369 * apu_stat.waveram_sample)) * 0.75; break;
						default: stream[x] = -32768; break;
					}
				}

				//Grab wave RAM step data for odd steps
				else
				{
					step >>= 1;
					apu_stat.waveram_sample = apu_stat.waveram_data[(apu_stat.waveram_bank << 3) + step] & 0xF;
	
					//Scale waveform to S16 audio stream
					switch(apu_stat.channel[2].volume)
					{
						case 0x0: stream[x] = -32768; break;
						case 0x1: stream[x] = -32768 + (4369 * apu_stat.waveram_sample); break;
						case 0x2: stream[x] = (-32768 + (4369 * apu_stat.waveram_sample)) * 0.5; break;
						case 0x3: stream[x] = (-32768 + (4369 * apu_stat.waveram_sample)) * 0.25; break;
						case 0x4: stream[x] = (-32768 + (4369 * apu_stat.waveram_sample)) * 0.75; break;
						default: stream[x] = -32768; break;
					}
				}
			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[2].sample_length == 0) && (!apu_stat.channel[2].length_flag)) { apu_stat.channel[2].sample_length = (apu_stat.channel[2].duration * 44100)/1000; }

			//Or stop sound after duration has been met, reset Sound 1 On Flag
			else if((apu_stat.channel[2].sample_length == 0) && (apu_stat.channel[2].length_flag)) 
			{ 
				stream[x] = -32768; 
				apu_stat.channel[2].sample_length = 0; 
				apu_stat.channel[2].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
	}
}

/****** Run APU for one cycle ******/
void APU::step() { }		

/****** SDL Audio Callback ******/ 
void audio_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;

	s16 channel_1_stream[length];
	s16 channel_2_stream[length];
	s16 channel_3_stream[length];

	APU* apu_link = (APU*) _apu;
	apu_link->generate_channel_1_samples(channel_1_stream, length);
	apu_link->generate_channel_2_samples(channel_2_stream, length);
	apu_link->generate_channel_3_samples(channel_3_stream, length);

	SDL_MixAudio((u8*)stream, (u8*)channel_1_stream, length*2, SDL_MIX_MAXVOLUME/16);
	SDL_MixAudio((u8*)stream, (u8*)channel_2_stream, length*2, SDL_MIX_MAXVOLUME/16);
	SDL_MixAudio((u8*)stream, (u8*)channel_3_stream, length*2, SDL_MIX_MAXVOLUME/16);
}

		