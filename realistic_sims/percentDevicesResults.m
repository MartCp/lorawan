close all;

% 1
r = dlmread('percentDevicesPlots/percentDevices-1.txt');
minDevs = 100;
increment = 200;
maxDevs = 900;
[X,Y] = meshgrid(minDevs:increment:maxDevs, 0:0.1:1);
Z = reshape(r(1:55,4), length(0:0.1:1), length(minDevs:increment:maxDevs));

figure;
contour(X,Y,Z, 'ShowText', 'on');

figure;
surf(X,Y,Z);

% 2
r = dlmread('percentDevicesPlots/percentDevices-2.txt');
minDevs = 100;
increment = 200;
maxDevs = 900;
[X,Y] = meshgrid(minDevs:increment:maxDevs, 0:0.1:1);
Z = reshape(r(1:55,4), length(0:0.1:1), length(minDevs:increment:maxDevs));

figure;
contour(X,Y,Z, 'ShowText', 'on');

figure;
surf(X,Y,Z);

% 4
r = dlmread('percentDevicesPlots/percentDevices-4.txt');
minDevs = 100;
increment = 500;
maxDevs = 1100;
[X,Y] = meshgrid(minDevs:increment:maxDevs, 0:0.1:1);
Z = reshape(r(1:33,4), length(0:0.1:1), length(minDevs:increment:maxDevs));

figure;
contour(X,Y,Z, 'ShowText', 'on');

figure;
surf(X,Y,Z);

% 8
r = dlmread('percentDevicesPlots/percentDevices-8.txt');
minDevs = 100;
increment = 500;
maxDevs = 1100;
[X,Y] = meshgrid(minDevs:increment:maxDevs, 0:0.1:1);
Z = reshape(r(1:33,4), length(0:0.1:1), length(minDevs:increment:maxDevs));

figure;
contour(X,Y,Z, 'ShowText', 'on');

figure;
surf(X,Y,Z);