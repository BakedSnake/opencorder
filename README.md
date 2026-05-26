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
corder --nogui --rate 48000 --output /path/to/file.wav
```

## LICENSE
MIT

