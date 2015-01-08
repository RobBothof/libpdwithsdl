/*
 *
Copyright (c) <2015> <Rob Bothof> <rbothof@xs4all.nl>

This is free software released into the public domain

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.
 
 * Description:
 *
 * An Example of Hooking LibPd-ouput into the SDL2 audio backend.
 * explained in the audioCallback functions.
 *
 * for messaging from and to libPD see the libPD cpp examples with pDOBject
 *
 *
 * Parts of this file are adaptations from the SDL2 audio examples by rerwarwar:
 * http://rerwarwar.weebly.com/sdl2-audio.html
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "PdBase.hpp"
#include <SDL.h>

using namespace std;
class pdManager;

//Screen dimension constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

class pdManager {
	public:
		int setup();
		void start();
		void shutdown();
        	void pdCallback(Uint8 *stream, int len);

	private:
        	pd::PdBase mPd;
        	pd::Patch mPatch;
        	SDL_AudioDeviceID audioDevId;
		static const int sampleRate = 48000;
		static const int numInputs = 0;
		static const int numOutputs = 2;
		static const int bufferSize = 512;
		int blockSize;
		int ticks;
        	float inbuf[0];
};

//function to pass the audioCallback to the pdManager class member function
//there could a prettier way to accomplish this, let me know..
void audioCallback(void* userdata, Uint8* stream, int len) {
    static_cast<pdManager*>(userdata)->pdCallback(stream, len);
}

void pdManager::pdCallback(Uint8* stream, int len) {
    	// here I connect libPD's output to SDL audio backend.
    	// The callback is automaticly invoked by SDL audio
    	// to request new samples once the audiodevice is unpaused.
    	// You can change the bufferSize for smaller latency at cpu cost.

   	// 'len' gives us the number of samples requested, depending
   	// on SDL's audioformat, I use 32bit float so I multiply by 4)
   	// len =( SDLBuffersize * number of channels * 4)

    	// I calculate how many pd blocks I need to fill SDL's audiobuffer
    	// the number of blocks is called 'ticks'
   	// so calculate the number of ticks/blocks for one SDL stream request

   	ticks = len / (blockSize * numOutputs * 4);

   	// prepare the buffer in float format

   	float* outbuf = (float*)stream;

   	// pump the audio buffer directly to SDL
   	// inbuf is ignored as SDL2 does not support audio input atm.
   	// but you could send wavefiles or generated audio from SDL to libPd
   	// in that case prepare the inbuf before calling processFloat

	mPd.processFloat(ticks, inbuf, outbuf );

}
void pdManager::start() {
    printf("SoundCheck one two\n");;
    mPd.computeAudio( true );
    SDL_PauseAudioDevice(audioDevId, 0); /* play! */
}
void pdManager::shutdown() {
    SDL_PauseAudioDevice(audioDevId, 1); /* pause device! */
    SDL_CloseAudioDevice(audioDevId);
    mPd.computeAudio(false);
    mPd.closePatch(mPatch);
}
int pdManager::setup() {
        //initialise libPD
        if ( !mPd.init(numInputs, numOutputs, sampleRate ) ) {
            printf("Could not init pd\n");
            return 1;
        }
        printf("PD Initialised\n");

        mPd.addToSearchPath("pd");

        // open patch
        mPatch = mPd.openPatch("test.pd", ".");
        blockSize=pd::PdBase::blockSize();


        //Initialize SDL Audio
        /* print the list of audio backends */
        printf("[SDL] %d audio backends compiled into SDL:", SDL_GetNumAudioDrivers());
        int i;
        for(i=0;i<SDL_GetNumAudioDrivers();i++)
            printf(" \n'%s\'", SDL_GetAudioDriver(i));
        printf("\n\n");

        /* attempt to init audio backend*/
        for(i=0;i<SDL_GetNumAudioDrivers();i++) {
            if (!SDL_AudioInit(SDL_GetAudioDriver(i))) {
                break;
            }
        }
        if(i == SDL_GetNumAudioDrivers()) {
            printf("[SDL] Failed to init audio: %s\n", SDL_GetError());
            return 1;
        }

        /* print the audio driver we are using */
        printf("[SDL] Audio driver: %s\n", SDL_GetCurrentAudioDriver());

        /* print the audio devices that we can see */
        printf("[SDL] %d audio devices:\n", SDL_GetNumAudioDevices(0));
        for(i = 0; i < SDL_GetNumAudioDevices(0); i++) {
            printf(" '%s\'\n", SDL_GetAudioDeviceName(i, 0)); /* again, 0 for playback */
        }

        SDL_AudioSpec want, have;
        SDL_zero(want);
        SDL_AudioDeviceID dev;
        /* a general specification */
        want.freq = sampleRate;
        want.format = AUDIO_F32;
        want.channels = numOutputs; /* 1, 2, 4, or 6 */
        want.samples = bufferSize; /* power of 2, or 0 and env SDL_AUDIO_SAMPLES is used */
        want.userdata = this;
        want.callback = audioCallback; /* can not be NULL */

        printf("[SDL] Desired - frequency: %d format: f %d s %d be %d sz %d channels: %d samples: %d\n", want.freq, SDL_AUDIO_ISFLOAT(want.format), SDL_AUDIO_ISSIGNED(want.format), SDL_AUDIO_ISBIGENDIAN(want.format), SDL_AUDIO_BITSIZE(want.format), want.channels, want.samples);
        /* open audio device, allowing any changes to the specification */
        audioDevId = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);

        if(!audioDevId) {
            printf("[SDL] Failed to open audio device: %s\n", SDL_GetError());
            return 1;
        }
        printf("[SDL] Obtained - frequency: %d format: f %d s %d be %d sz %d channels: %d samples: %d\n", have.freq, SDL_AUDIO_ISFLOAT(have.format), SDL_AUDIO_ISSIGNED(have.format), SDL_AUDIO_ISBIGENDIAN(have.format), SDL_AUDIO_BITSIZE(have.format), have.channels, have.samples);

        return 0;

}



int main(int argc, char **argv) {
	//The window we'll be rendering to
	SDL_Window* window = NULL;

	//The surface contained by the window
	SDL_Surface* screenSurface = NULL;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 )
	{
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
	}
	else
	{

		//Create window
		window = SDL_CreateWindow( "SDL width LibPD", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if( window == NULL )
		{
			printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		}
		else
		{
			//Get window surface
			screenSurface = SDL_GetWindowSurface( window );

			//Fill the surface white
			SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0x22, 0x00, 0x00 ) );

			//Update the surface
			SDL_UpdateWindowSurface( window );


            //Setup LibPD and SDL_Audio
            pdManager pdMan;
            if (!pdMan.setup()) {;

                pdMan.start();
                SDL_Delay( 10000 );


                pdMan.shutdown();
            }

        //Destroy window
        SDL_DestroyWindow( window );
		}

	SDL_Quit();
    }
  return 0;
}


