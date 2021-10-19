### About
32 bit clone of PDP-1 Minskytron

https://www.masswerk.at/minskytron/

https://www.masswerk.at/minskytron/minskytron-annotated.txt

### Usage

```
wget https://github.com/dtschump/CImg/raw/master/CImg.h
g++ -I. -Ofast -march=native mtronpp.cpp -o mtronpp -pthread -lX11
or
g++ -I. -Ofast -march=native -Dcimg_use_openmp=1 -fopenmp -ggdb3 tronpp.cpp -o mtronpp -pthread -lX11
```

### TODO

- [ ] deal with synchronization
- [ ] support arbitrary resolution
- [ ] port to xscreensaver
