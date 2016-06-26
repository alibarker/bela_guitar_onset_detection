#include <Bela.h>
#include <cmath>
#include <ne10/NE10.h>
#include "StringOnsetDetector.h"
#include <vector>
#include <OSCClient.h>

// network stuff

OSCClient oscClient;



// parameters
const int numStrings = 6;
const int bufferSize = 512;
const int hopSize = 256;
const float threshold = 30;
const float timeThresholdInS = 0.1;


// prev samples (for dc blocker)
float prev[numStrings];

// sample count for hop
int sampleCount;

// detectors
std::vector<StringOnsetDetector> env;
//StringOnsetDetector* env;


// HELPER FUNCTIONS

void sendNoteOnMessage(int stringNum, float amplitude)
{
    oscClient.queueMessage(oscClient.newMessage.to("/kellycaster/noteon").add(stringNum).add(amplitude).end());
    
//    rt_printf("String %d on: %f", stringNum, amplitude);
}



void processEnvelopeOutputs(float* amplitudes, int size)
{
//    int numTriggered = 0;
//    int loudestString = 0;
//    
//    for (int i = 0; i < size; ++i)
//    {
//        if (amplitudes[i] > 0.0)
//        {
//            numTriggered++;
//            if (amplitudes[i] > amplitudes[loudestString])
//            {
//                loudestString = i;
//            }
//        }
//    }
//    
//    for (int i = 0; i < size; ++i) {
//        if (amplitudes[i] < 0.25 * amplitudes[loudestString]) {
//            amplitudes[i] = 0.0;
//        }
//    }
//    
}

// SETUP

bool setup(BelaContext *context, void *userData)
{
    sampleCount = 0;
    
    float sampleRate = context->analogSampleRate;
    int hopTheshold = floor((timeThresholdInS * sampleRate) / (float) hopSize);
    printf("hop threshold: %d \n", hopTheshold);
    
    for (int i = 0; i < numStrings; i++)
    {
        
        
        env.push_back(StringOnsetDetector(bufferSize, threshold, hopTheshold));
        
        prev[i] = 0;
    }
    
    
    oscClient.setup(3000, "192.168.7.1");
    
    sendNoteOnMessage(100, 100);

    printf("Finishing Setup\n");
    
	return true;
}


// RENDER

void render(BelaContext *context, void *userData)
{

    
    float noteOnAmplitude[numStrings];

	for(unsigned int n = 0; n < context->analogFrames; n++) {
		
        // process input for all strings
        
		for (int i = 0; i < numStrings; i++)
		{
            // read in input
		    float input = analogRead(context, n, i);
            

            
            // apply dc blocker and send sample to
            // envelope detector
		    env[i].processSample(input - prev[i]);
            prev[i] = input;
            
		}
		
        // increment sample count
	    sampleCount++;
		    
        // once sample count has been reached   
        // trigger envelope calculation
		if (sampleCount >= hopSize)
		{
            sampleCount = 0;
		    for (int i = 0; i < numStrings; i++)
		    {
                // check if note on and send message
                noteOnAmplitude[i] = env[i].calculateEnv();
            }
            
            processEnvelopeOutputs(noteOnAmplitude, numStrings);
            
            for (int i = 0; i < numStrings; ++i) {
                if (noteOnAmplitude[i] > 0.0)
                    sendNoteOnMessage(i, noteOnAmplitude[i]);
            }
		}
	}
}



void cleanup(BelaContext *context, void *userData)
{

}
