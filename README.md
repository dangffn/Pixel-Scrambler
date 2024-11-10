# Scrambler

This program scrambles images with a specified password.

For the test image below:

![image](./test.jpg)

Run the following:

```
scrambler scramble ./test.jpg 1234
```

This will create a `./test.scrambled.jpg` image with all of the pixels scrambled seemingly randomly.

![image](./test.scrambled.png)

You can then run:

```
scrambler unscramble ./test.scrambled.jpg 1234
```

Which will unscramble the image.

## Dependencies

- `libopencv-dev`

## Build

```
make
```