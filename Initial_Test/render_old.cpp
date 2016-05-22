#include <BeagleRT.h>
#include <cmath>
#include <newScope.h>
#include <NE10.h> 
#include <StringOnsetDetector.h>

newScope scope;

const int numStrings = 1;

float toOdf[numStrings];

int hopSize;
int sampleCount;

int prev[numStrings];

const int bufferSize = 512;

std::vector<StringOnsetDetector> env;

bool setup(BeagleRTContext *context, void *userData)
{
	scope.setup(numStrings, context->analogSampleRate);

	hopSize = 256;
	
    sampleCount = 0;
    for (int i = 0; i < numStrings; i++)
    {
        env.push_back(StringOnsetDetector(bufferSize));
        prev[i] = 0;
    }

	return true;
}


// RENDER

void render(BeagleRTContext *context, void *userData)
{

	for(unsigned int n = 0; n < context->analogFrames; n++) {
		
		// Get input and dc block
		float in[numStrings];

		for (int i = 0; i < numStrings; i++)
		{
		    in[i] = analogReadFrame(context, n, i);
		    toOdf[i] = in[i] - prev[i];
		    prev[i] = in[i];
		    
		    env[i].processSample(toOdf[i]);
		    

		    
		}
		
	    sampleCount++;
		    
		if (sampleCount >= hopSize)
		{
		    for (int i = 0; i < numStrings; i++)
		    {
		        bool noteOn = env[i].calculateEnv();
		        if(noteOn) {
		            rt_printf("Note %d On: %f, %f, %f\n", i, env[i].output, env[i].prevOut1, env[i].prevOut2);
		        }
		            
		    }
		}
		
// 		scope.log(env[0].output);

	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().

void cleanup(BeagleRTContext *context, void *userData)
{

}
