close all;

r = dlmread('percentDevicesPlots/percentDevices.txt');
[X,Y] = meshgrid(200:200:2000, 0:0.05:1);
Z = reshape(r(:,3), length(0:0.05:1), length(200:200:2000));

figure;
surf (X,Y,Z, 'EdgeColor', 'none');

figure;
contour(X,Y,Z, 'ShowText', 'on');