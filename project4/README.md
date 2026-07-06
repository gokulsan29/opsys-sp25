# opsys-sp25-project4-starter

This is the starter code for Project 4 in CSE 30341 Spring 2025 at Notre Dame.

The key code here [gcode2png](gcode2png) is derived from [Nautilus-Thumbnailer_GCode](https://github.com/Spiritdude/Nautilus_Thumbnailer_GCode) and used as a simulated printer.

To install:

```
conda create -n opsys-project4
conda activate opsys-project4
conda install -c conda-forge povray
```

Then try this example:

```
./gcode2png tests/cube.gcode --output=cube.png
```
