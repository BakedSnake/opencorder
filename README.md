# Opencorder - A pipewire audio recorder
An internal audio recorder that uses a pipewire monitor to record audio that is currently being played.

## Note
This project is still a work in progress. Once there is a stable build, it will be added as a release.

## Dependencies
- [pipewire](https://www.pipewire.org/)
- [libsndfile](https://libsndfile.github.io/libsndfile/)

## Build
```bash
make
make config # optional config file for defaults
```

## Install
```bash
make install
```

## Usage
Graphical User Interface:
```bash
corder
```

CLI:
```bash
corder --nogui --rate 48000 --channels 2 --output /path/to/file.wav # 48K sampling rate stereo
corder -n -r 44100 -c 1 -o /path/to/file.wav                        # 44100 Hz sampling rate mono
```

```bash
corder --version
```

```bash
corder --help
```

## LICENSE
MIT

