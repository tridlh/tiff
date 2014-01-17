image process on tiff container
====

read tiff, process, write to out.tiff

supported process: 
image rotate: 90/180/270 degree clockwise

build: 
	gcc 1.c
run:   
	./a.out -i <filename> -r <number>

e.g. rotate res/jupiter.tif 90 degree and save rotated image to out.tiff:
	./a.out -i res/jupiter.tif -r 90
