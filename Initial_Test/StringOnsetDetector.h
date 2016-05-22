
// Helper functions

#include <NE10.h>

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


class StringOnsetDetector
{
public:
	StringOnsetDetector(const int size);
	~StringOnsetDetector();

	float calculateEnv();
	void processSample(float in)
	{
		inputBuffer[writePointer] = in;
		writePointer++;
		if (writePointer >= fftSize)
		{
			writePointer = 0;
		}
	}

	float output;
	float prevOut1;
	float prevOut2;

private:
	ne10_fft_cpx_float32_t* timeDomain;
	ne10_fft_cpx_float32_t* freqDomain;
	ne10_float32_t* absFreqDomain;
	ne10_float32_t* prevAbsFreqDomain;
	ne10_float32_t* temp1;
	float* inputBuffer;
	int writePointer;

	 int fftSize;
	 ne10_fft_cfg_float32_t cfg;
	 bool notFirstFrame;

};

StringOnsetDetector::StringOnsetDetector(const int size)
{
    fftSize = size;
	inputBuffer = new float[fftSize];
    timeDomain = (ne10_fft_cpx_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_fft_cpx_float32_t));
    freqDomain = (ne10_fft_cpx_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_fft_cpx_float32_t));
    absFreqDomain = (ne10_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_float32_t));
	prevAbsFreqDomain = (ne10_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_float32_t));
    temp1 = (ne10_float32_t*) NE10_MALLOC (fftSize * sizeof (ne10_float32_t));

    cfg = ne10_fft_alloc_c2c_float32 (fftSize);
    output = prevOut1 = prevOut2 = 0;
    writePointer = 0;

    memset(inputBuffer, 0, fftSize * sizeof (float));
    notFirstFrame = false;

}
StringOnsetDetector::~StringOnsetDetector()
{
    // delete inputBuffer;
    // NE10_FREE(timeDomain);
    // NE10_FREE(freqDomain);
    // NE10_FREE(absFreqDomain);
    // NE10_FREE(prevAbsFreqDomain);
    // NE10_FREE(temp1);

}

float StringOnsetDetector::calculateEnv()
{
	// env output
	bool isNoteOn = false;

	// temp pointer
	int pointer = writePointer;

	prevOut2 = prevOut1;
	prevOut1 = output;

	// copy buffer to timeDomain
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

	ne10_fft_c2c_1d_float32_neon(freqDomain, timeDomain, cfg->twiddles, cfg->factors, fftSize, 0);

	// get abs value of each bin
	complex_abs(absFreqDomain, freqDomain, fftSize);

	// ignore first frame and then perform ODF
	if (notFirstFrame)
	{

		ne10_sub_float_neon(temp1, absFreqDomain, prevAbsFreqDomain, fftSize);
		half_wave_rectify(temp1, fftSize);
		output = sum_vector(temp1, fftSize);

	} else { notFirstFrame = true;}

	// copy current abs to previous abs
	ne10_float32_t* temp;

	temp = absFreqDomain;
	absFreqDomain = prevAbsFreqDomain;
	prevAbsFreqDomain = temp;

	if (prevOut1 > output && prevOut1 > prevOut2 && prevOut1 > 10)
	{
		isNoteOn = true;
	}

	return isNoteOn;
}

