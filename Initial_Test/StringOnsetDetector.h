#include <NE10.h>

// VECTOR HELPER FUNCTIONS

void half_wave_rectify(ne10_float32_t* src, int size)
{
	for (int i = 0; i < size; ++i)
		src[i] = (float) src[i] > 0 ? src[i] : 0;
}

void complex_abs(ne10_float32_t* dst, ne10_fft_cpx_float32_t* src, int size)
{
	for (int i = 0; i < size; ++i)
		dst[i] = sqrt(src[i].r * src[i].r + src[i].i * src[i].i);
}

float sum_vector(ne10_float32_t* src, int size)
{
	float output = 0;
	for (int i = 0; i < size; ++i)
		output += src[i];

	return output;
}

// STRING ONSET DETECTOR CLASS

class StringOnsetDetector
{
public:
	StringOnsetDetector(const int size, const float thresh, const int timeThsresh);
	~StringOnsetDetector();
    
    // function that takes the previous fftSize inputs and
    // returns on if it detects an onset
	float calculateEnv();
    
    // incrementally applies input to the input buffer
	void processSample(float in)
	{
		inputBuffer[writePointer] = in;
        
        // wrap pointer
		writePointer++;
		if (writePointer >= fftSize)
			writePointer = 0;
	}

    // current and previous outputs
	float output;
	float prevOut1;
	float prevOut2;

private:
    // buffers used for fft calculation
	ne10_fft_cpx_float32_t* timeDomain;
	ne10_fft_cpx_float32_t* freqDomain;
    
    // buffers used for
	ne10_float32_t* absFreqDomain;
	ne10_float32_t* prevAbsFreqDomain;
	ne10_float32_t* temp1;
    
    // fft config options
    ne10_fft_cfg_float32_t cfg;
    
    // input buffer and pointer
	float* inputBuffer;
	int writePointer;
    
    // parameters
    int fftSize;
    float threshold;
    unsigned int timeThresholdInHops;

    // state variables
    bool notFirstFrame;
    unsigned long hopsSinceLastNoteOn;
    
    // helper functions
    
    void swapBuffers(ne10_float32_t* buffer1, ne10_float32_t* buffer2)
    {
        ne10_float32_t* temp;
        
        temp = buffer1;
        buffer1 = buffer2;
        buffer2 = temp;
    }
    
    float detectOnset(float in1, float in2, float in3, float thresh)
    {
        if (in2 > in1 && in2 > in3 && in2 > thresh && hopsSinceLastNoteOn > timeThresholdInHops)
        {
            hopsSinceLastNoteOn = 0;
            return in2;
        }
        else
        {
            hopsSinceLastNoteOn++;
            return 0.0;
        }
    }


};


StringOnsetDetector::StringOnsetDetector(const int size, const float thresh, const int timeThresh) :  fftSize(size),
                                                                                threshold(thresh),
                                                                                timeThresholdInHops(timeThresh)
{
	
    printf("Begin adding string detector \n");

    // initalise buffers
    inputBuffer = new float[fftSize];
    memset(inputBuffer, 0, fftSize * sizeof (float));

    timeDomain =        (ne10_fft_cpx_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_fft_cpx_float32_t));
    freqDomain =        (ne10_fft_cpx_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_fft_cpx_float32_t));

    absFreqDomain =     (ne10_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_float32_t));
	prevAbsFreqDomain = (ne10_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_float32_t));
    temp1 =             (ne10_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_float32_t));
//
    output = prevOut1 = prevOut2 = 0;
    writePointer = 0;

    // inialise fft
    cfg = ne10_fft_alloc_c2c_float32_neon (fftSize);


    // initialise state variables
    notFirstFrame = false;
    hopsSinceLastNoteOn = 0;
    printf("Finish adding string detector \n");


}

StringOnsetDetector::~StringOnsetDetector()
{
//     delete inputBuffer;
//     NE10_FREE(timeDomain);
//     NE10_FREE(freqDomain);
//     NE10_FREE(absFreqDomain);
//     NE10_FREE(prevAbsFreqDomain);
//     NE10_FREE(temp1);

}



float StringOnsetDetector::calculateEnv()
{
    
	// temp pointer
	int pointer = writePointer;

	prevOut2 = prevOut1;
	prevOut1 = output;

	// copy buffer to timeDomain buffer
	for (int n = 0; n < fftSize; ++n)
	{
		timeDomain[n].r = (ne10_float32_t) inputBuffer[pointer];
		timeDomain[n].i = 0;
        
		pointer++;
		if (pointer >= fftSize)
		{
			pointer = 0;
		}
	}

	// perform fft

	ne10_fft_c2c_1d_float32_neon(freqDomain, timeDomain, cfg, 0);

	// get abs value of each bin
	complex_abs(absFreqDomain, freqDomain, fftSize);

	// ignore first frame and then perform ODF
	if (notFirstFrame)
	{
        // sub tract abs from previous abs
		ne10_sub_float_neon(temp1, absFreqDomain, prevAbsFreqDomain, fftSize);
        
        // half wave rectify
		half_wave_rectify(temp1, fftSize);
        
        // sum
		output = sum_vector(temp1, fftSize);

	} else { notFirstFrame = true;}

	// copy current abs to previous abs
    swapBuffers(absFreqDomain, prevAbsFreqDomain);


    
    // detect onset
	return detectOnset(output, prevOut1, prevOut2, threshold);
}

