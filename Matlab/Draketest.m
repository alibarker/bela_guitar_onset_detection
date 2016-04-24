clear all
close all

fid = fopen('../out.bin', 'r');
A = fread(fid, 'float');
Fs = 44100;

A = A(3: length(A));

frame = 2048;
hop = 256;

numframes = floor((length(A) - frame)/hop);

t = [0: numframes - 1] .* hop ./ Fs;


env = sf(A, frame, hop);
plot(t, env, 'bx-');
xlim([2.2 3.6]);