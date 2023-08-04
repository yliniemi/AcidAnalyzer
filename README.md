# Acid Analyzer

The most accurate open source specrum analyzer. Check this video to see what I'm talking about.
https://www.youtube.com/watch?v=7z2ppEhz4gE

The analyzer works well on systems that use Pipewire by default.
These include Debian 12 and above, Ubuntu 22.10 and above and Arch Linux (if you choose Pipewire when installing).
I tried adding support for PulseAudio and ALSA but got frustrated by their interfaces. Pipewire is much more robust and sensible.

The spectrum is drawn in the terminal using ncurses. I'm planning on supporting GLFW later. Perhaps making a proper mouse controlled graphical interface. But that's far in the future.


## Installation

```console
git clone https://github.com/yliniemi/AcidAnalyzer
AcidAnalyzer/AcidAnalyzer/build/AcidAnalyzer
```


## Compilation

### Prerequisites for Debian 12 and above

```console
sudo apt install cmake pkg-config libpipewire-0.3-dev libfftw3-dev libncurses-dev
```


### Actual compilation

```console
git clone https://github.com/yliniemi/AcidAnalyzer
cd AcidAnalyzer/AcidAnalyzer/build/
cmake ..
make
./AcidAnalyzer
```


## Interfacing

### Command line arguments

Example
```console
./AcidAnalyzer --fps 122 --FFTsize 16384 --dynamicRange 60 --kaiserBeta 5.0
```

### Keyboard shortcuts

These don't exist yet either. I will start by making Q quit the program. Currently you have to press ctrl + C



