function [ output ] = sf(input, framesize, jumpsize)
% HFC Function
numFrames = floor((length(input)-framesize)/jumpsize);

k = [0:framesize/2 - 1 zeros(1, framesize/2)]';

previous_fft = zeros(framesize, 1);

output = zeros(numFrames, 1);

    for n = 1:numFrames
        framestart = (n-1) * jumpsize + 1;
        framestop = framestart + framesize - 1;

        input_fft = fft(input(framestart:framestop));
        
        output(n) = sum( max(abs(input_fft) - abs(previous_fft), 0) );
        
        previous_fft = input_fft;
    end
end

