#include <iostream>
#include <deque>
#include <thread>

#include <CImg.h>
using namespace cimg_library;

#include "timer.hpp"

// wget https://github.com/dtschump/CImg/raw/master/CImg.h
// g++ -I. -Ofast -march=native mtronpp.cpp -o mtronpp -pthread -lX11

// g++ -I. -Ofast -march=native -Dcimg_use_openmp=1 -fopenmp -ggdb3 mtronpp.cpp -o mtronpp -pthread -lX11

/*
https://www.masswerk.at/minskytron/
https://www.masswerk.at/minskytron/minskytron-annotated.txt

ya := ya + ((xa + xb) >> sh0) // sh0 = 1 + Test Word bits 0..2
xa := xa – ((ya – yb) >> sh1) // sh1 = 1 + Test Word bits 3..5
display a dot at xa | ya

yb := yb + ((xb – xc) >> sh2) // sh2 = 1 + Test Word bits 6..8
xb := xb – ((yb – yc) >> sh3) // sh3 = 1 + Test Word bits 9..11
display a dot at xb | yb

yc := yc + ((xc – xa) >> sh4) // sh4 = 1 + Test Word bits 12..14
xc := xc – ((yc – ya) >> sh5) // sh5 = 1 + Test Word bits 15..17
display a dot at xc | yc

xa0,	dpy i 17770  // (730007 - 10000) + 17770 = 737777 = -0137016 (1's complement!)
xb0,	0            // 0
xc0,	and          // 020000
ya0,	0            // 0
yb0,	ior          // 060000
yc0,	0            // 0
*/

struct odot {
    int x, y;
    odot(int _x, int _y) : x(_x), y(_y) {};
};

std::deque<odot> oa, ob, oc;

int maxodots = 1024*6;
int filler_sleep = 100;
double gm = -1;

CImg<unsigned char> visu(1024,1024,1,3,0);
const unsigned char
    red[] = { 200,255,50 },
    green[] = { 150,255,150 },
    blue[] = { 50,255,200 },
    grey[] = { 80,80,80 },
    yellow[] = { 255,255,0 };
CImgDisplay disp(visu,"tube");

//unsigned int tb = 0b011000111001110011100010000010;
//unsigned int tb = 0b001100011100111001110001000001 << 0; // original
  unsigned int tb = 0b001110011100111001110001000001 << 0;
//unsigned int tb = 0b001100010100011001110001100001 << 0;
//                  ....5....5....5....5....5....5

int ya, xa, yb, xb, yc, xc;

int sh0, sh1,
    sh2, sh3,
    sh4, sh5;

// constatnt shift add
int CSA = 1;

// x, y coordinates shift walue (keep 10 bits to wrap around 1024 screen pixels)
#define SB (32-10)

// initial constant multiplier
#define ICM 10

// shift value bit width
#define SVBW 5

// display bit rectangle width in pixels
#define DBW 20

template<typename T>
void drawdot(int x, int y, double o, T c) {
    visu.draw_point((x>>SB)+512,
                    (y>>SB)+512,
                    c, o);

    auto co = std::max(.0, o-0.3);
    visu.draw_circle((x>>SB)+512,
                     (y>>SB)+512,
                     1, c, co);

}

void draw_test_bits(int N)
{
    int w = DBW, shift = 0;
    const unsigned char *c;
    auto ltb = tb;
    for (int i=0; i<N; i++) {
        if (ltb & 1<<(6*SVBW-1))
            c = yellow;
        else
            c = grey;
        visu.draw_rectangle(i*DBW, 0,
                            (i+1)*DBW-2, 6,
                            c, 0.5);
        if (i>0 && i%SVBW==0) {
            visu.draw_circle(i*DBW-DBW/2, 10, 2, red, 0.5);
        }
        ltb <<= 1;
    }
}

void reinit() {
    visu.fill(0);
    ya=0; xa=0737777<<ICM;
    yb=060000<<ICM; xb=0;
    yc=0; xc=020000<<ICM;
    sh0 = ((tb >> SVBW*5) & 0b11111) + CSA;
    sh1 = ((tb >> SVBW*4) & 0b11111) + CSA;
    sh2 = ((tb >> SVBW*3) & 0b11111) + CSA;
    sh3 = ((tb >> SVBW*2) & 0b11111) + CSA;
    sh4 = ((tb >> SVBW*1) & 0b11111) + CSA;
    sh5 = ((tb >> SVBW*0) & 0b11111) + CSA;

    oa.clear();
    ob.clear();
    oc.clear();
}

CImg<unsigned char> box(4,4,1,1,1);

int bit_from_mouse_x() {
    const int x = disp.mouse_x();
    int bit = 30-1-x/DBW;
    //std::cout << bit << std::endl;
    return bit;
}

void dot_filler() {
    while (!disp.is_closed() && !disp.is_keyESC()) {

        ya += (xa + xb) >> sh0;
        xa -= (ya - yb) >> sh1;

        yb += (xb - xc) >> sh2;
        xb -= (yb - yc) >> sh3;

        yc += (xc - xa) >> sh4;
        xc -= (yc - ya) >> sh5;

        oa.emplace_back(xa, ya);
        ob.emplace_back(xb, yb);
        oc.emplace_back(xc, yc);

        while (oa.size() > maxodots) {
            oa.pop_front();
            ob.pop_front();
            oc.pop_front();
        }

        usleep(filler_sleep);
    }
}

int main() {
    //cimg::openmp_mode(1);
    reinit();

    Timer t;
    double d = 1.01;
    unsigned frame = 0;
    unsigned i = 0;
    int b = 0, oldb = 0, oldwheel;
    char text[1024] = {0};

    std::thread filler(dot_filler);

    while (!disp.is_closed() && !disp.is_keyESC()) {
        Timer ft;
        //main_disp.wait();
        b = disp.button()&1;
        if ( oldb > b && disp.mouse_y()>=0) {
            tb ^= (1<<bit_from_mouse_x());
            reinit();
        }
        oldb = b;

        auto wheel = disp.wheel();
        if (oldwheel != wheel) {
            bool direction = oldwheel < wheel;
            switch(bit_from_mouse_x()) {
                case 0:
                    CSA += direction ? 1 : -1; break;
                case 1:
                    d += direction ? (d-1)/2 : -(d-1)/2; break;
                case 2:
                    filler_sleep += direction ? 10 : -10; break;
                case 4:
                    maxodots += direction ? 10 : -10; break;
                case 5:
                    gm += direction ? 0.1 : -0.1; break;
            }
            oldwheel = wheel;
            reinit();
        }


        if (t.get() > 1./60.) {
            auto a = oa, b = ob, c = oc;

            unsigned s = std::min(a.size(), std::min(b.size(), c.size()));

            visu.fill(0);
            //visu = visu.blur_box(2, 1);
            //visu = visu.blur_median(2, 2);
            for (unsigned i = 0; i < s; i++) {
                double N = s;
                double ii = i;
                double o = (1-std::exp(-ii*gm/N))/(1-std::exp(-gm));
                if (o<0) o = 0;
                //if (i<100) o = 1;
                drawdot(a.at(i).x, a.at(i).y, o, red);
                drawdot(b.at(i).x, b.at(i).y, o, green);
                drawdot(c.at(i).x, c.at(i).y, o, blue);
            }
            /*
            for (auto &p : a)
                drawdot(p.x, p.y, red);
            for (auto &p : b)
                drawdot(p.x, p.y, green);
            for (auto &p : c)
                drawdot(p.x, p.y, blue);
            */
            //visu /= d;
            draw_test_bits(6*SVBW);

            //sprintf(text, "CSA:%d filler_sleep:%d maxodots:%d", CSA, filler_sleep, maxodots);
            visu.draw_text(2, 20,
                "CSA:%d filler_sleep:%d maxodots:%d gm:%.2f",
                yellow, 0, 1, 13,
                CSA, filler_sleep, maxodots, gm);

            visu.display(disp);
            t.reset();
            frame = 0;
        }

    }

    filler.join();
    return 0;
}
