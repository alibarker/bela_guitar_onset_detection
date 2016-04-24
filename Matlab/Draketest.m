clear all
close all

fid = fopen('out.bin', 'r');
A = fread(fid, 'float');
Fs = 44100;

frame = 512;
hop = 128;

numframes = floor((length(A) - frame)/hop);

t = [0: numframes - 1] .* hop ./ Fs;


env = sf(A, frame, hop);
plot(t, env, 'bx-');