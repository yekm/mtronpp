#include <iostream>
#include <deque>
#include <thread>
#include <mutex>
#include <bitset>

#include <CImg.h>

using namespace cimg_library;

#include "timer.hpp"

struct odot {
    int ax, ay, bx, by, cx, cy;
};

typedef std::deque<odot> osc_type;
osc_type osc;

std::mutex meh; // meh

size_t maxodots = 1024*6;
int filler_sleep = 100;
double gm = -2.5;

#define W 1024
#define H 1024

CImg<unsigned char> visu(W,H,1,3,0);
const unsigned char
    lred[] = { 200,255,50 },
    lgreen[] = { 150,255,150 },
    lblue[] = { 50,255,200 },
    red[] = { 255,0,0 },
    green[] = { 0,255,0 },
    blue[] = { 0,0,255 },
    grey[] = { 80,80,80 },
    white[] = { 255,255,255 },
    yellow[] = { 255,255,0 };
CImgDisplay disp(visu,"tube");

//unsigned int tb = 0b011000111001110011100010000010;
//unsigned int tb = 0b001100011100111001110001000001; // original
  unsigned int tb = 0b001110011100111001110001000001;
//unsigned int tb = 0b000110001100111001110001000010; // alt1
//unsigned int tb = 0b001100010100011001110001100001;
//                    ....|....|....|....|....|....|

int ya, xa, yb, xb, yc, xc;

int sh0, sh1,
    sh2, sh3,
    sh4, sh5;

// constatnt shift add
int CSA = 1;

// bits left
#define BL 12
// x, y coordinates shift walue (keep 10 bits to wrap around 1024 screen pixels)
#define SB (32-BL)

// initial constant multiplier
#define ICM 10

// shift value bit width
#define SVBW 5

// display bit rectangle width in pixels
#define DBW 24

template<typename T>
void drawdot(int x, int y, double o, T c) {
    x = x>>SB;
    y = y>>SB;

    double smax = ~(~unsigned(0) << BL) + 1;

    double scalex = W/smax;
    double scaley = H/smax;

    //scalex /= double(W)/H;

    int sx = scalex*x + W/2;
    int sy = scaley*y + H/2;

    visu.draw_point(sx, sy, c, o);

    auto cc = std::max(.0, o-0.3);
    visu.draw_circle(sx, sy, 1, c, cc);
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
        if (i%SVBW==0 && i>0) {
            visu.draw_rectangle(i*DBW-1, 0,
                                i*DBW+1, 8,
                                white, 0.9);
        }
        ltb <<= 1;
    }
}

void reinit() {
    visu.fill(0);
    ya=0; xa=0737777<<ICM;
    yb=060000<<ICM; xb=0;
    yc=0; xc=020000<<ICM;

    std::bitset<30> t(tb);
    std::cout << t << std::endl;

    auto mask = ~(~unsigned(0) << SVBW);
    sh0 = ((tb >> SVBW*5) & mask) + CSA;
    sh1 = ((tb >> SVBW*4) & mask) + CSA;
    sh2 = ((tb >> SVBW*3) & mask) + CSA;
    sh3 = ((tb >> SVBW*2) & mask) + CSA;
    sh4 = ((tb >> SVBW*1) & mask) + CSA;
    sh5 = ((tb >> SVBW*0) & mask) + CSA;

    {
        std::lock_guard<std::mutex> l(meh);
        osc.clear();
    }

}

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

        {
            std::lock_guard<std::mutex> l(meh);
            osc.emplace_back(odot{xa, ya, xb, yb, xc, yc});

            if (osc.size() > maxodots)
                osc.erase(osc.begin(), osc.end() - maxodots);
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

    std::thread filler(dot_filler);

    while (!disp.is_closed() && !disp.is_keyESC()) {
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
                    maxodots += direction ? 100 : -100; break;
                case 5:
                    gm += direction ? 0.1 : -0.1; break;
            }
            oldwheel = wheel;
            reinit();
        }


        osc_type _osc;
        if (t.get() > 1./60.) { // ~60 fps
            {
                std::lock_guard<std::mutex> l(meh);
                _osc = osc;
            }

            visu.fill(0);
            const double N = _osc.size();
            double i = 0;

            for (auto & oi : _osc) {
                double o = (1-std::exp(-i*gm/N))/(1-std::exp(-gm));
                if (o<0) o=0;

                drawdot(oi.ax, oi.ay, o, lred);
                drawdot(oi.bx, oi.by, o, lgreen);
                drawdot(oi.cx, oi.cy, o, lblue);
                ++i;
            }

            draw_test_bits(6*SVBW);

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
