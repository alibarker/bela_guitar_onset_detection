/*
 * render.cpp
 *
 *  Created on: Oct 24, 2014
 *      Author: parallels
 */


#include <BeagleRT.h>
#include <cmath>
#include <Utilities.h>
#include <WriteFile.h>


 WriteFile file1;



float* buffer;
int bufferSize;
int writePointer;

int sampleCount;
int hopeSize;
// setup() is called once before the audio rendering starts.
// Use it to perform any initialisation and allocation which is dependent
// on the period size or sample rate.
//
// userData holds an opaque pointer to a data structure that was passed
// in from the call to initAudio().
//
// Return true on success; returning false halts the program.

bool setup(BeagleRTContext *context, void *userData)
{

	file1.init("out.bin"); //set the file name to write to
	file1.setEchoInterval(1000);
	file1.setFileType(kBinary);
	file1.setFormat("%.4f,");

	bufferSize = 2048;

	buffer = new float[bufferSize];

	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.

void calculateEnv()
{

}

void render(BeagleRTContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
		// float inL = audioReadFrame(context, n, 0);
		float inR = audioReadFrame(context, n, 1);

		buffer[writePointer] = inL;

		writePointer++;
		writePointer = (writePointer + bufferSize) % bufferSize;

		sampleCount++;

		if (sampleCount == hopSize)
		{
			calculateEnv();
		}



	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().

void cleanup(BeagleRTContext *context, void *userData)
{

}
