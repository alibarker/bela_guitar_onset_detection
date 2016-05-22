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
#include <NE10.h> 

 WriteFile file1;

ne10_fft_cpx_float32_t* timeDomain;
ne10_fft_cpx_float32_t* freqDomain;
ne10_float32_t* absFreqDomain;
ne10_float32_t* prevAbsFreqDomain;
ne10_float32_t* temp1;

ne10_fft_cfg_float32_t cfg;

bool notFirstFrame = false;

float* buffer;
int bufferSize;
int writePointer;
int frameRatio;

float out_1, out_2;

int sampleCount;
int hopSize;
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

	file1.init("samples.bin"); //set the file name to write to
	file1.setEchoInterval(1000);
	file1.setFileType(kText);
	file1.setFormat("%.4f,");

	bufferSize = 2048;
	hopSize = 256;

	buffer = new float[bufferSize];
    timeDomain = (ne10_fft_cpx_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_fft_cpx_float32_t));
    freqDomain = (ne10_fft_cpx_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_fft_cpx_float32_t));
    absFreqDomain = (ne10_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_float32_t));
	prevAbsFreqDomain = (ne10_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_float32_t));
    temp1 = (ne10_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_float32_t));

    frameRatio = context->audioFrames / context->analogFrames;
    
    cfg = ne10_fft_alloc_c2c_float32 (bufferSize);
    out_1 = out_2 = 0;
    writePointer = 0;

    memset(buffer, 0, bufferSize * sizeof (float));

	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.

void half_wave_rectify(ne10_float32_t* src, int size)
{
	for (int i = 0; i < size; ++i)
	{
		src[i] = (float) src[i] > 0 ? src[i] : 0;
	}
}

void complex_abs(ne10_float32_t* dst, ne10_fft_cpx_float32_t* src, int size)
{
	for (int i = 0; i < size; ++i)
	{
		dst[i] = sqrt(src[i].r * src[i].r + src[i].i * src[i].i);
		// rt_printf("%f ", dst[i]);
	}

	// rt_printf("\n");
}

float sum_vector(ne10_float32_t* src, int size)
{
	float output = 0;
	for (int i = 0; i < size; ++i)
	{
		output += src[i];
	}

	return output;
}

float calculateEnv()
{

	// rt_printf("Calculate Started\n");
	// wrap buffer
	float output;
	int pointer = writePointer;

	for (int n = 0; n < bufferSize; ++n)
	{
		timeDomain[n].r = (ne10_float32_t) buffer[pointer];
		timeDomain[n].i = 0;

		// TODO (task 1): update "pointer" each time and wrap it around to keep
		// it within the circular buffer
		pointer++;
		if (pointer >= bufferSize)
		{
			pointer = 0;
		}
	}

	ne10_fft_c2c_1d_float32_neon(freqDomain, timeDomain, cfg->twiddles, cfg->factors, bufferSize, 0);

	// rt_printf("FFT Calculated\n");
	// rt_printf("FFT Bin 1 Real: %f\t Im: %f\n", freqDomain[1].r, freqDomain[1].i);
	complex_abs(absFreqDomain, freqDomain, bufferSize);
	// rt_printf("Abs FFT Bin 1: %f\n", absFreqDomain[1]);



	if (notFirstFrame)
	{

		//output = sum( max(abs(input_fft) - abs(previous_fft), 0) );

		ne10_sub_float_neon(temp1, absFreqDomain, prevAbsFreqDomain, bufferSize);

		// rt_printf("X - Xprev: %f\n", temp1[1]);



		half_wave_rectify(temp1, bufferSize);
		// rt_printf("Rectified: %f\n", temp1[1]);

		output = sum_vector(temp1, bufferSize);
		// rt_printf("Output: %f\n", output);


	} else { notFirstFrame = true;}

	// rt_printf("Spectral Flux: %f\n", output);

	// copy current to previous
	ne10_float32_t* temp;

	temp = absFreqDomain;
	absFreqDomain = prevAbsFreqDomain;
	prevAbsFreqDomain = temp;

	return output;
}

void render(BeagleRTContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
		// float inL = audioReadFrame(context, n, 0);
		float inR = audioReadFrame(context, n, 1);
		float out_0;
        float in0;
        
        
        if (n == 0) {
            rt_printf("Ana In 0: %f\n", in0);
        }
        in0 = analogReadFrame(context, n, 0);
        rt_printf("Ana In 0: %f\n", inR);
        
        
		buffer[writePointer] = inR;

		// file1.log(inR);	

		writePointer++;

		if (writePointer >= bufferSize)
		{
			writePointer = 0;
		}



		sampleCount++;

		if (sampleCount == hopSize - 1)
		{
			out_0 = calculateEnv();


			sampleCount = 0;
			// rt_printf("Out_0: %f\n", out_0);


			if (out_1 > out_0 && out_1 > out_2 && out_1 > 10)
			{
				rt_printf("NOTE ON\n");

			}

			out_2 = out_1;
			out_1 = out_0;
		}

	

	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().

void cleanup(BeagleRTContext *context, void *userData)
{
    NE10_FREE (timeDomain);
    NE10_FREE (freqDomain);
    NE10_FREE (cfg);
}
