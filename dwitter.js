c.width=c.height=1024
w2=1024/2
h2=1024/2
x.fillRect(0,0,1024,1024)
if (t==0) {
  sh0=sh1=sh2=sh3=8
  sh4=3
  sh5=2
  x1b=0xefffc00
  y1b=x2b=y3b=0
  y2b=1800000
  x3b=800000
  sx=c.width/(1<<12)
  sy=c.height/(1<<12)

  m=32
  N=100*m
  gm=-3
  speed=64
}

function fr(xx, yy) {
  x.fillRect((xx>>21)*sx+512, (yy>>21)*sy+256, 2, 2)
}

x1=x1b;y1=y1b;x2=x2b;y2=y2b;x3=x3b;y3=y3b

for(i=0;i<N;i++) {
  y1 += (x1 + x2) >> sh0
  x1 -= (y1 - y2) >> sh1

  y2 += (x2 - x3) >> sh2
  x2 -= (y2 - y3) >> sh3

  y3 += (x3 - x1) >> sh4
  x3 -= (y3 - y1) >> sh5

  if (i==speed) {
    x1b=x1;y1b=y1;x2b=x2;y2b=y2;x3b=x3;y3b=y3
  }
  o = (1-Math.exp(-i*gm/N))/(1-Math.exp(-gm));
  x.fillStyle = 'hsl(90, 90%, '+o*100+'%)'
  fr(x1,y1)
  fr(x2,y2)
  fr(x3,y3)
}
