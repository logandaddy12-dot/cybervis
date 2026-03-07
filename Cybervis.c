/*
 * CYBERVIS v5.0 — Terminal Spectral Engine
 * Build: gcc -O3 -o cybervis cybervis.c -lm -lpthread
 *
 * Usage: cybervis [help] [options]
 *   cybervis help
 *   cybervis -m <mode> -c <color> -s <1-10> -d <1-10> -f <fps> --no-status
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdint.h>

/* ─── ANSI ───────────────────────────────────────────────── */
#define ESC "\033"
#define CSI ESC "["
#define HIDE_CURSOR  CSI"?25l"
#define SHOW_CURSOR  CSI"?25h"
#define ALT_ON       ESC"[?1049h"
#define ALT_OFF      ESC"[?1049l"

/* ─── output buffer ──────────────────────────────────────── */
#define OBUF (1<<22)
static char obuf[OBUF];
static int  olen=0;
static inline void ob_raw(const char*s,int n){if(olen+n>=OBUF){write(1,obuf,olen);olen=0;}memcpy(obuf+olen,s,n);olen+=n;}
static inline void ob_str(const char*s){ob_raw(s,strlen(s));}
static inline void ob_flush(void){if(olen){write(1,obuf,olen);olen=0;}}
static char _f[80];
#define OFG(r,g,b) (snprintf(_f,80,CSI"38;2;%d;%d;%dm",r,g,b),ob_str(_f))
#define OBG(r,g,b) (snprintf(_f,80,CSI"48;2;%d;%d;%dm",r,g,b),ob_str(_f))
#define OGO(r,c)   (snprintf(_f,80,CSI"%d;%dH",r,c),ob_str(_f))
#define ORESET     ob_str(CSI"0m")
#define OBOLD      ob_str(CSI"1m")
#define OCLEAR     ob_str(CSI"2J")
#define OHOME      ob_str(CSI"H")

/* ─── globals ────────────────────────────────────────────── */
static volatile int running=1,mode=0,palette=0,paused=0;
static volatile int spd=5,density=5,show_hud=1,bold_heads=1,inverted=0;
static int W=0,H=0,fps_cap=60;
static struct termios orig_termios;

/* ─── 22 PALETTES ────────────────────────────────────────── */
typedef struct{int r,g,b;}RGB;
typedef struct{RGB hi,mid,lo;const char*name;}Pal;
static Pal pals[]={
    {{180,255,180},{  0,220, 80},{  0, 55, 15},"PHOSPHOR" },
    {{180,240,255},{  0,180,255},{  0, 40,110},"ICE"      },
    {{255,160,160},{255, 20, 60},{100,  0, 20},"BLOOD"    },
    {{255,255,150},{255,190,  0},{110, 70,  0},"SOLAR"    },
    {{220,150,255},{150,  0,255},{ 55,  0,110},"ULTRA"    },
    {{ 50,255,200},{  0,200,180},{  0, 70, 60},"NEON"     },
    {{220,220,220},{140,140,140},{ 50, 50, 50},"MONO"     },
    {{255,140, 40},{200, 80,  0},{ 80, 30,  0},"EMBER"    },
    {{140,255,255},{ 40,200,200},{  0, 80, 80},"ARCTIC"   },
    {{255,200,255},{200, 80,200},{ 80,  0, 80},"SAKURA"   },
    {{200,255,100},{100,200,  0},{ 40, 80,  0},"ACID"     },
    {{255,255,255},{180,180,255},{ 60, 60,160},"GHOST"    },
    {{255,120, 80},{200, 50, 20},{ 80, 15,  0},"LAVA"     },
    {{100,200,255},{  0,100,200},{  0, 30, 80},"OCEAN"    },
    {{255,230,100},{200,160,  0},{ 80, 60,  0},"GOLD"     },
    {{180,255,120},{  0,255, 60},{  0,100, 20},"TOXIC"    },
    {{255, 80,180},{200,  0,100},{ 80,  0, 40},"PINK"     },
    {{120,120,255},{ 40, 40,220},{  0,  0, 90},"COBALT"   },
    {{255,200,150},{180,100, 50},{ 70, 30,  0},"COPPER"   },
    {{150,255,200},{  0,200,120},{  0, 70, 40},"MINT"     },
    {{255,100,100},{180,  0,  0},{ 70,  0,  0},"CRIMSON"  },
    {{200,200,100},{130,130,  0},{ 50, 50,  0},"KHAKI"    },
};
#define NPALS 22
static RGB phi(void){RGB c=pals[palette].hi; if(inverted){c.r=255-c.r;c.g=255-c.g;c.b=255-c.b;}return c;}
static RGB pmi(void){RGB c=pals[palette].mid;if(inverted){c.r=255-c.r;c.g=255-c.g;c.b=255-c.b;}return c;}
static RGB plo(void){RGB c=pals[palette].lo; if(inverted){c.r=255-c.r;c.g=255-c.g;c.b=255-c.b;}return c;}

/* ─── timing ─────────────────────────────────────────────── */
static double now_ms(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec*1000.0+ts.tv_nsec/1e6;}
static void get_size(void){struct winsize ws;if(ioctl(1,TIOCGWINSZ,&ws)==0){W=ws.ws_col;H=ws.ws_row;}}
static void enable_raw(void){tcgetattr(0,&orig_termios);struct termios r=orig_termios;r.c_lflag&=~(ECHO|ICANON|ISIG);r.c_cc[VMIN]=0;r.c_cc[VTIME]=0;tcsetattr(0,TCSANOW,&r);}
static void cleanup(void){tcsetattr(0,TCSANOW,&orig_termios);printf(SHOW_CURSOR ALT_OFF CSI"0m\n");fflush(stdout);}
static void on_signal(int s){(void)s;running=0;}
static void on_resize(int s){(void)s;get_size();OCLEAR;OHOME;}

/* ─── cell grid ──────────────────────────────────────────── */
typedef struct{const char*ch;int r,g,b;}Cell;
static Cell*grid=NULL,*pgrid=NULL;
static void grid_alloc(void){free(grid);free(pgrid);grid=calloc(H*W,sizeof(Cell));pgrid=calloc(H*W,sizeof(Cell));}
#define CE(r,c) grid[(r)*W+(c)]
#define PE(r,c) pgrid[(r)*W+(c)]
static void grid_render(void){
    for(int r=0;r<H-1;r++)for(int c=0;c<W;c++){
        Cell*cu=&CE(r,c),*pr=&PE(r,c);
        if(cu->r==pr->r&&cu->g==pr->g&&cu->b==pr->b&&cu->ch==pr->ch)continue;
        OGO(r+1,c+1);
        if(!cu->ch||cu->ch[0]==' '||(cu->r+cu->g+cu->b==0)){ORESET;ob_str(" ");}
        else{OFG(cu->r,cu->g,cu->b);ob_str(cu->ch);}
        *pr=*cu;
    }
}
static void grid_fade(float f){
    for(int i=0;i<H*W;i++){
        grid[i].r=(int)(grid[i].r*f);grid[i].g=(int)(grid[i].g*f);grid[i].b=(int)(grid[i].b*f);
        if(grid[i].r<3&&grid[i].g<3&&grid[i].b<3){grid[i].ch=" ";grid[i].r=grid[i].g=grid[i].b=0;}
    }
}

/* ─── charsets ───────────────────────────────────────────── */
static const char*KANA[]={"ア","イ","ウ","エ","オ","カ","キ","ク","ケ","コ","サ","シ","ス","セ","ソ","タ","チ","ツ","テ","ト","ナ","ニ","ヌ","ネ","ノ","ハ","ヒ","フ","ヘ","ホ","マ","ミ","ム","メ","モ","ヤ","ユ","ヨ","ラ","リ","ル","レ","ロ","ワ","ヲ","ン","0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F","<",">","|","*","+","-","=","~","▓","░","╬","╫","╪","▲","▼","◆","■","●"};
#define NKANA (sizeof(KANA)/sizeof(KANA[0]))
static const char*rk(void){return KANA[rand()%NKANA];}
static const char*BRAILLE[]={"⠁","⠂","⠃","⠄","⠅","⠆","⠇","⠈","⠉","⠊","⠋","⠌","⠍","⠎","⠏","⠐","⠑","⠒","⠓","⠔","⠕","⠖","⠗","⠘","⠙","⠚","⠛","⠜","⠝","⠞","⠟","⠠","⠡","⠢","⠣","⠤","⠥","⠦","⠧","⠨","⠩","⠪","⠫","⠬","⠭","⠮","⠯","⠰","⠱","⠲","⠳","⠴","⠵","⠶","⠷","⠸","⠹","⠺","⠻","⠼","⠽","⠾","⠿"};
#define NBRAILLE 64
static const char*BLOCKS[]={" ","▁","▂","▃","▄","▅","▆","▇","█"};
static const char*WAVEC[]={"▁","▂","▃","▄","▅","▆","▇","█","▇","▆","▅","▄","▃","▂","▁"};
static const char*DOTS[]={" ","·","∙","•","◦","○","◉","●","◈","◆","◇","░","▒","▓","█"};

/* ══════════════════════════════════════════════════════════
   MODE 0 — MATRIX RAIN
══════════════════════════════════════════════════════════ */
#define MAXDROPS 1024
typedef struct{int y,col,speed,len,bright;}Drop;
static Drop drops[MAXDROPS];static int ndrop=0;
static void matrix_init(void){
    ndrop=0;int nc=W*density/5;if(nc>MAXDROPS)nc=MAXDROPS;
    for(int i=0;i<nc;i++){drops[i].col=rand()%W;drops[i].y=-(rand()%(H*4));drops[i].speed=1+rand()%3;drops[i].len=6+rand()%(20+density*2);drops[i].bright=(rand()%5==0);ndrop++;}
}
static void matrix_tick(void){
    RGB h=phi();
    grid_fade(0.75f+(10-spd)*0.02f);
    int tk=1+spd/4;
    for(int t=0;t<tk;t++)for(int i=0;i<ndrop;i++){
        Drop*d=&drops[i];int row=d->y/4;
        if(row>=0&&row<H-1&&d->col<W){
            CE(row,d->col).ch=rk();
            if(d->bright&&bold_heads){CE(row,d->col).r=255;CE(row,d->col).g=255;CE(row,d->col).b=255;}
            else{CE(row,d->col).r=h.r;CE(row,d->col).g=h.g;CE(row,d->col).b=h.b;}
        }
        d->y+=d->speed;
        if(d->y/4-d->len>H){d->y=-(rand()%(H*4));d->speed=1+rand()%3;d->len=6+rand()%(20+density*2);d->bright=(rand()%5==0);d->col=rand()%W;}
    }
}

/* ══════════════════════════════════════════════════════════
   MODE 1 — AUDIO BARS + WAVEFORM
══════════════════════════════════════════════════════════ */
#define FFT_N 2048
#define NBANDS 128
static float ab_sm[NBANDS],ab_pk[NBANDS],ab_ph[NBANDS];
static float ao_fr[16],ao_am[16],ao_ph2[16],ao_dr[16];
static float araw[FFT_N];static double audio_t=0;
static void fft_c(float*re,float*im,int n){
    for(int i=1,j=0;i<n;i++){int bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j){float t;t=re[i];re[i]=re[j];re[j]=t;t=im[i];im[i]=im[j];im[j]=t;}}
    for(int len=2;len<=n;len<<=1){float ang=-2.0f*(float)M_PI/len,wR=cosf(ang),wI=sinf(ang);for(int i=0;i<n;i+=len){float cR=1,cI=0;for(int j=0;j<len/2;j++){float uR=re[i+j],uI=im[i+j],vR=re[i+j+len/2]*cR-im[i+j+len/2]*cI,vI=re[i+j+len/2]*cI+im[i+j+len/2]*cR;re[i+j]=uR+vR;im[i+j]=uI+vI;re[i+j+len/2]=uR-vR;im[i+j+len/2]=uI-vI;float nR=cR*wR-cI*wI;cI=cR*wI+cI*wR;cR=nR;}}}
}
static void audio_init(void){for(int i=0;i<16;i++){ao_fr[i]=50+(rand()%4000);ao_am[i]=0.05f+(rand()%100)/400.0f;ao_ph2[i]=(rand()%1000)/1000.0f*2*(float)M_PI;ao_dr[i]=((rand()%200)-100)/60000.0f;}memset(ab_sm,0,sizeof(ab_sm));memset(ab_pk,0,sizeof(ab_pk));memset(ab_ph,0,sizeof(ab_ph));}
static void audio_tick(void){
    static float re[FFT_N],im[FFT_N];float sr=44100,dt=1/sr;
    for(int s=0;s<FFT_N;s++){float v=0;for(int i=0;i<16;i++){ao_ph2[i]+=2*(float)M_PI*ao_fr[i]*dt;ao_fr[i]+=ao_dr[i];if(ao_fr[i]<30){ao_fr[i]=30;ao_dr[i]*=-1;}if(ao_fr[i]>10000){ao_fr[i]=10000;ao_dr[i]*=-1;}v+=sinf(ao_ph2[i])*ao_am[i];}float beat=fmodf((float)(audio_t+s*dt)*2,1);if(beat<0.03f)v+=0.6f*(1-beat/0.03f);float w=0.5f*(1-cosf(2*(float)M_PI*s/(FFT_N-1)));re[s]=v*w;im[s]=0;araw[s]=v;}
    audio_t+=FFT_N*dt;
    for(int i=0;i<16;i++){ao_am[i]+=((rand()%100-50))/80000.0f;if(ao_am[i]<0.01f)ao_am[i]=0.01f;if(ao_am[i]>0.5f)ao_am[i]=0.5f;}
    fft_c(re,im,FFT_N);
    for(int b=0;b<NBANDS;b++){float f0=20*powf(1000,b/(float)NBANDS),f1=20*powf(1000,(b+1)/(float)NBANDS);int i0=(int)(f0/sr*FFT_N),i1=(int)(f1/sr*FFT_N);if(i1<=i0)i1=i0+1;if(i1>=FFT_N/2)i1=FFT_N/2-1;float mag=0;for(int i=i0;i<=i1;i++){float m=sqrtf(re[i]*re[i]+im[i]*im[i]);if(m>mag)mag=m;}float v=mag/(FFT_N/4)*(density/5.0f);if(v>1)v=1;if(v>ab_sm[b])ab_sm[b]=v;else ab_sm[b]*=0.85f-(spd-5)*0.01f;if(ab_sm[b]>ab_pk[b]){ab_pk[b]=ab_sm[b];ab_ph[b]=(float)(25+spd*3);}else{if(ab_ph[b]>0)ab_ph[b]--;else ab_pk[b]*=0.96f;}}
}
static void audio_render(void){
    RGB h=phi(),m=pmi(),l=plo();int vh=H-3,bw=W/NBANDS;if(bw<1)bw=1;int nb=W/bw;if(nb>NBANDS)nb=NBANDS;
    for(int b=0;b<nb;b++){float fr=ab_sm[b];int full=(int)(fr*vh),blk=(int)((fr*vh-full)*8+0.5f);if(blk>8)blk=8;int bx=b*bw+1;
        for(int r=0;r<vh;r++){int sr2=H-2-r;if(sr2<1||sr2>=H)continue;OGO(sr2,bx);
            if(r<full){float t=(float)r/vh;OFG((int)(l.r+t*(h.r-l.r)),(int)(l.g+t*(h.g-l.g)),(int)(l.b+t*(h.b-l.b)));for(int w=0;w<bw;w++)ob_str("█");}
            else if(r==full){OFG(m.r,m.g,m.b);for(int w=0;w<bw;w++)ob_str(BLOCKS[blk]);}
            else{ORESET;for(int w=0;w<bw;w++)ob_str(" ");}
            int pr=H-2-(int)(ab_pk[b]*vh);if(sr2==pr){OGO(sr2,bx);OFG(255,255,255);for(int w=0;w<bw;w++)ob_str("─");}}}
    OGO(2,1);OFG(h.r,h.g,h.b);for(int c=0;c<W;c++){float v=araw[(int)((float)c/W*FFT_N)];int i=(int)((v+1)*0.5f*14);if(i<0)i=0;if(i>14)i=14;ob_str(WAVEC[i]);}
    OGO(3,1);OFG(l.r,l.g,l.b);for(int c=0;c<W;c++){float v=-araw[(int)((float)c/W*FFT_N)];int i=(int)((v+1)*0.5f*14);if(i<0)i=0;if(i>14)i=14;ob_str(WAVEC[i]);}
}

/* ══════════════════════════════════════════════════════════
   MODE 2 — GLITCH STORM
══════════════════════════════════════════════════════════ */
typedef struct{int row,col,w,h,r,g,b,life,ml,type,shift;}GBlk;
#define MAXG 400
static GBlk gb[MAXG];static int ng=0;
static const char*GC="!@#$%^&*<>{}[]|/?~`XZQW01░▒▓╬╫╪";
static void glitch_spawn(void){if(ng>=MAXG)return;RGB h=phi(),m=pmi();GBlk*b=&gb[ng++];b->row=rand()%H+1;b->col=rand()%W+1;b->w=rand()%(W/2)+2;b->h=rand()%4+1;b->life=b->ml=rand()%15+2;b->type=rand()%6;b->shift=(rand()%41)-20;int w=rand()%4;if(w==0){b->r=h.r;b->g=h.g;b->b=h.b;}else if(w==1){b->r=255;b->g=50;b->b=50;}else if(w==2){b->r=50;b->g=200;b->b=255;}else{b->r=m.r;b->g=m.g;b->b=m.b;}}
static void glitch_tick(void){int rate=density*spd/3+2;for(int i=0;i<rate;i++)glitch_spawn();for(int i=0;i<ng;){gb[i].life--;if(gb[i].life<=0)gb[i]=gb[--ng];else i++;}}
static int gfr=0;
static void glitch_render(void){
    RGB h=phi(),l=plo();gfr++;
    int sc=W*H*density/(600/spd+1);
    for(int i=0;i<sc;i++){int r=rand()%(H-1)+1,c=rand()%W+1;OGO(r,c);float a=(rand()%100)/200.0f;OFG((int)(l.r*a),(int)(l.g*a),(int)(l.b*a));ob_raw(&GC[rand()%33],1);}
    for(int i=0;i<ng;i++){GBlk*b=&gb[i];float fd=(float)b->life/b->ml;OFG((int)(b->r*fd),(int)(b->g*fd),(int)(b->b*fd));int col=b->col+b->shift;if(col<1)col=1;for(int row=b->row;row<b->row+b->h&&row<H;row++){OGO(row,col);for(int w=0;w<b->w&&col+w<=W;w++){if(b->type==0)ob_str("█");else if(b->type==1)ob_raw(&GC[rand()%33],1);else if(b->type==2)ob_str("▬");else if(b->type==3)ob_str("▪");else if(b->type==4)ob_str("╳");else ob_str("░");}}}
    if(gfr%70<5){static const char*WD[]={"CORRUPT","SEGFAULT","0xDEAD","KERNEL PANIC","NULL PTR","OVERFLOW","SIGKILL","BUS ERROR","DIVIDE BY ZERO","STACK SMASH"};const char*wd=WD[(gfr/70)%10];int col=W/2-(int)strlen(wd)/2;if(col<1)col=1;OFG(h.r,h.g,h.b);OBOLD;OGO(H/2+(rand()%3-1),col);ob_str(wd);ORESET;}
    if(gfr%200<2){OBG(h.r,h.g,h.b);for(int r=1;r<H;r++){OGO(r,1);for(int c=0;c<W;c++)ob_str(" ");}ORESET;}
}

/* ══════════════════════════════════════════════════════════
   MODE 3 — NEURAL WEB
══════════════════════════════════════════════════════════ */
#define MAXN 200
typedef struct{float x,y,vx,vy,pulse,pspd,energy;}Nod;
static Nod nds[MAXN];static int nnds=0;
static void neural_init(void){nnds=40+density*10;if(nnds>MAXN)nnds=MAXN;for(int i=0;i<nnds;i++){nds[i].x=(rand()%1000)/1000.0f;nds[i].y=(rand()%1000)/1000.0f;nds[i].vx=((rand()%200)-100)/80000.0f*(spd*0.4f);nds[i].vy=((rand()%200)-100)/80000.0f*(spd*0.4f);nds[i].pulse=(rand()%1000)/1000.0f*2*(float)M_PI;nds[i].pspd=0.01f+(rand()%100)/2500.0f;nds[i].energy=(rand()%100)/100.0f;}}
static void draw_line(int r0,int c0,int r1,int c1,int rr,int gg,int bb){int dr=abs(r1-r0),dc=abs(c1-c0),sr=r0<r1?1:-1,sc=c0<c1?1:-1,err=dr-dc;while(1){if(r0>=0&&r0<H-1&&c0>=0&&c0<W){Cell*cl=&CE(r0,c0);if(rr+gg+bb>cl->r+cl->g+cl->b){cl->r=rr;cl->g=gg;cl->b=bb;if(dr==0)cl->ch="─";else if(dc==0)cl->ch="│";else if(sr==sc)cl->ch="╲";else cl->ch="╱";}}if(r0==r1&&c0==c1)break;int e2=2*err;if(e2>-dc){err-=dc;r0+=sr;}if(e2<dr){err+=dr;c0+=sc;}}}
static void neural_tick(void){
    RGB m=pmi(),h=phi();grid_fade(0.7f);float sf=(float)spd/5;
    for(int i=0;i<nnds;i++){Nod*n=&nds[i];n->x+=n->vx*sf;n->y+=n->vy*sf;if(n->x<0){n->x=0;n->vx*=-1;}if(n->x>1){n->x=1;n->vx*=-1;}if(n->y<0){n->y=0;n->vy*=-1;}if(n->y>1){n->y=1;n->vy*=-1;}n->pulse+=n->pspd*sf;n->energy+=((rand()%100-50)/6000.0f);if(n->energy<0)n->energy=0;if(n->energy>1)n->energy=1;}
    float md=0.15f+density*0.03f;
    for(int i=0;i<nnds;i++)for(int j=i+1;j<nnds;j++){float dx=nds[i].x-nds[j].x,dy=(nds[i].y-nds[j].y)*2,dist=sqrtf(dx*dx+dy*dy);if(dist>=md)continue;float s=1-dist/md,p=0.5f+0.5f*sinf(nds[i].pulse+nds[j].pulse),br=s*p*0.85f;draw_line((int)(nds[i].y*H),(int)(nds[i].x*W),(int)(nds[j].y*H),(int)(nds[j].x*W),(int)(m.r*br),(int)(m.g*br),(int)(m.b*br));}
    for(int i=0;i<nnds;i++){Nod*n=&nds[i];int r=(int)(n->y*H),c=(int)(n->x*W);if(r<0||r>=H-1||c<0||c>=W)continue;float p=0.5f+0.5f*sinf(n->pulse);CE(r,c).ch="◈";CE(r,c).r=(int)(h.r*p);CE(r,c).g=(int)(h.g*p);CE(r,c).b=(int)(h.b*p);}
}

/* ══════════════════════════════════════════════════════════
   MODE 4 — FIRE
══════════════════════════════════════════════════════════ */
static float*fbuf=NULL;
static void fire_init(void){free(fbuf);fbuf=calloc(H*W,sizeof(float));}
static void fire_tick(void){
    for(int c=0;c<W;c++)fbuf[(H-2)*W+c]=(rand()%100)/100.0f*density/5.0f;
    float dc=0.04f+(10-spd)*0.008f;
    for(int r=0;r<H-2;r++)for(int c=0;c<W;c++){int bl=r+1+rand()%2;if(bl>=H-1)bl=H-2;int lc=c+(rand()%3)-1;if(lc<0)lc=0;if(lc>=W)lc=W-1;float v=fbuf[bl*W+lc]-dc*(rand()%100)/100.0f;if(v<0)v=0;fbuf[r*W+c]=v;}
    static const char*FC[]={" ","░","░","▒","▒","▓","▓","█","█"};
    for(int r=0;r<H-1;r++)for(int c=0;c<W;c++){float v=fbuf[r*W+c];if(v<0.01f){CE(r,c).ch=" ";CE(r,c).r=CE(r,c).g=CE(r,c).b=0;continue;}int ci=(int)(v*8);if(ci>8)ci=8;CE(r,c).ch=FC[ci];if(v<0.25f){CE(r,c).r=(int)(v*4*180);CE(r,c).g=0;CE(r,c).b=0;}else if(v<0.5f){float t=(v-0.25f)*4;CE(r,c).r=180+(int)(t*75);CE(r,c).g=(int)(t*100);CE(r,c).b=0;}else if(v<0.75f){float t=(v-0.5f)*4;CE(r,c).r=255;CE(r,c).g=100+(int)(t*155);CE(r,c).b=0;}else{float t=(v-0.75f)*4;CE(r,c).r=255;CE(r,c).g=255;CE(r,c).b=(int)(t*220);}}
}

/* ══════════════════════════════════════════════════════════
   MODE 5 — WAVE INTERFERENCE
══════════════════════════════════════════════════════════ */
static float wt=0;
typedef struct{float x,y,freq,amp,phase;}WSrc;
#define NWS 5
static WSrc ws[NWS];
static void wave_init(void){for(int i=0;i<NWS;i++){ws[i].x=(rand()%100)/100.0f;ws[i].y=(rand()%100)/100.0f;ws[i].freq=1.5f+(rand()%30)/10.0f;ws[i].amp=0.3f+(rand()%40)/100.0f;ws[i].phase=(rand()%100)/100.0f*2*(float)M_PI;}}
static void wave_tick(void){
    RGB h=phi(),l=plo();wt+=0.04f*(spd/5.0f);
    for(int i=0;i<NWS;i++){ws[i].x+=sinf(wt*0.3f+i)*0.001f*(spd/5.0f);ws[i].y+=cosf(wt*0.2f+i*1.3f)*0.001f*(spd/5.0f);if(ws[i].x<0)ws[i].x=0;if(ws[i].x>1)ws[i].x=1;if(ws[i].y<0)ws[i].y=0;if(ws[i].y>1)ws[i].y=1;}
    for(int r=0;r<H-1;r++)for(int c=0;c<W;c++){float fx=(float)c/W,fy=(float)r/(H-1),v=0;for(int i=0;i<NWS;i++){float dx=fx-ws[i].x,dy=(fy-ws[i].y)*2,d=sqrtf(dx*dx+dy*dy);v+=ws[i].amp*sinf(d*ws[i].freq*20-wt*2+ws[i].phase);}v=(v+NWS*0.3f)/(NWS*0.6f+0.01f);if(v<0)v=0;if(v>1)v=1;v*=density/5.0f;if(v>1)v=1;int ci=(int)(v*14);if(ci<0)ci=0;if(ci>14)ci=14;CE(r,c).ch=DOTS[ci];CE(r,c).r=(int)(l.r+v*(h.r-l.r));CE(r,c).g=(int)(l.g+v*(h.g-l.g));CE(r,c).b=(int)(l.b+v*(h.b-l.b));}
}

/* ══════════════════════════════════════════════════════════
   MODE 6 — BRAILLE NOISE / PLASMA
══════════════════════════════════════════════════════════ */
static float pt=0;
static void plasma_tick(void){
    RGB h=phi(),m=pmi(),l=plo();pt+=0.03f*(spd/5.0f);
    for(int r=0;r<H-1;r++)for(int c=0;c<W;c++){
        float cx=(float)c/W,cy=(float)r/(H-1);
        float v=sinf(cx*12+pt)+sinf(cy*9+pt*1.3f)+sinf((cx+cy)*7+pt*0.7f)+sinf(sqrtf((cx-0.5f)*(cx-0.5f)+(cy-0.5f)*(cy-0.5f))*22-pt*2);
        v=(v+4)/8; if(v<0)v=0;if(v>1)v=1;
        float br=powf(fabsf(sinf(v*(float)M_PI*3+pt)),0.4f)*(density/5.0f);if(br>1)br=1;
        int ci=(int)(br*NBRAILLE);if(ci>=NBRAILLE)ci=NBRAILLE-1;
        CE(r,c).ch=BRAILLE[ci];
        CE(r,c).r=(int)(l.r+br*(h.r-l.r));CE(r,c).g=(int)(l.g+br*(h.g-l.g));CE(r,c).b=(int)(l.b+br*(h.b-l.b));
    }
}

/* ══════════════════════════════════════════════════════════
   MODE 7 — STARFIELD / WARP
══════════════════════════════════════════════════════════ */
#define NSTARS 400
typedef struct{float x,y,z,pz;}Star;
static Star stars[NSTARS];
static void stars_init(void){for(int i=0;i<NSTARS;i++){stars[i].x=(rand()%2000-1000)/100.0f;stars[i].y=(rand()%2000-1000)/100.0f;stars[i].z=(rand()%1000)/100.0f+0.1f;stars[i].pz=stars[i].z;}}
static void stars_tick(void){
    RGB h=phi(),m=pmi();
    grid_fade(0.4f);
    float speed=(float)spd*0.015f;
    for(int i=0;i<NSTARS*(density/5.0f+0.2f)&&i<NSTARS;i++){
        Star*s=&stars[i];s->pz=s->z;s->z-=speed;
        if(s->z<=0){s->x=(rand()%2000-1000)/100.0f;s->y=(rand()%2000-1000)/100.0f;s->z=10;s->pz=10;continue;}
        int sx=(int)((s->x/s->z+1)*W/2),sy=(int)((s->y/s->z+1)*(H-1)/2);
        int px=(int)((s->x/s->pz+1)*W/2),py=(int)((s->y/s->pz+1)*(H-1)/2);
        if(sx<0||sx>=W||sy<0||sy>=H-1)continue;
        float bright=1.0f-s->z/10.0f;
        /* draw trail */
        if(px>=0&&px<W&&py>=0&&py<H-1){CE(py,px).ch="·";CE(py,px).r=(int)(m.r*bright*0.4f);CE(py,px).g=(int)(m.g*bright*0.4f);CE(py,px).b=(int)(m.b*bright*0.4f);}
        const char*sc=bright>0.8f?"★":bright>0.5f?"✦":bright>0.3f?"·":"·";
        CE(sy,sx).ch=sc;CE(sy,sx).r=(int)(h.r*bright);CE(sy,sx).g=(int)(h.g*bright);CE(sy,sx).b=(int)(h.b*bright);
    }
}

/* ══════════════════════════════════════════════════════════
   MODE 8 — CELLULAR AUTOMATA (Game of Life style but visual)
══════════════════════════════════════════════════════════ */
static uint8_t*ca_grid=NULL,*ca_next=NULL;
static int ca_gen=0;
static void ca_init(void){
    free(ca_grid);free(ca_next);
    ca_grid=calloc(H*W,1);ca_next=calloc(H*W,1);ca_gen=0;
    int alive=(int)(H*W*density*0.04f);
    for(int i=0;i<alive;i++)ca_grid[rand()%(H*W)]=1;
}
static void ca_tick(void){
    RGB h=phi(),m=pmi(),l=plo();
    ca_gen++;
    /* B3/S23 (Conway) with slight variation */
    for(int r=0;r<H-1;r++)for(int c=0;c<W;c++){
        int nb=0;
        for(int dr=-1;dr<=1;dr++)for(int dc=-1;dc<=1;dc++){if(!dr&&!dc)continue;int nr=(r+dr+H)%(H-1),nc=(c+dc+W)%W;nb+=ca_grid[nr*W+nc];}
        int cur=ca_grid[r*W+c];
        ca_next[r*W+c]=(cur&&(nb==2||nb==3))||(!cur&&nb==3)?1:0;
        if(CE(r,c).r>3){CE(r,c).r=(int)(CE(r,c).r*0.85f);CE(r,c).g=(int)(CE(r,c).g*0.85f);CE(r,c).b=(int)(CE(r,c).b*0.85f);}
        if(ca_next[r*W+c]&&!ca_grid[r*W+c]){CE(r,c).ch="█";CE(r,c).r=h.r;CE(r,c).g=h.g;CE(r,c).b=h.b;}
        else if(!ca_next[r*W+c]&&ca_grid[r*W+c]){CE(r,c).ch="░";CE(r,c).r=l.r;CE(r,c).g=l.g;CE(r,c).b=l.b;}
        else if(ca_next[r*W+c]){CE(r,c).ch="▓";CE(r,c).r=m.r;CE(r,c).g=m.g;CE(r,c).b=m.b;}
    }
    uint8_t*tmp=ca_grid;ca_grid=ca_next;ca_next=tmp;
    /* re-seed occasionally */
    if(ca_gen%200==0){int seeds=5+rand()%10;for(int i=0;i<seeds;i++)ca_grid[rand()%((H-1)*W)]=1;}
}

/* ══════════════════════════════════════════════════════════
   MODE 9 — MATRIX RAIN (BRAILLE variant — ultra dense)
══════════════════════════════════════════════════════════ */
static void braille_rain_tick(void){
    RGB h=phi();
    grid_fade(0.78f+(10-spd)*0.02f);
    int tk=1+spd/4;
    for(int t=0;t<tk;t++)for(int i=0;i<ndrop;i++){
        Drop*d=&drops[i];int row=d->y/4;
        if(row>=0&&row<H-1&&d->col<W){
            CE(row,d->col).ch=BRAILLE[rand()%NBRAILLE];
            if(d->bright&&bold_heads){CE(row,d->col).r=255;CE(row,d->col).g=255;CE(row,d->col).b=255;}
            else{CE(row,d->col).r=h.r;CE(row,d->col).g=h.g;CE(row,d->col).b=h.b;}
        }
        d->y+=d->speed;
        if(d->y/4-d->len>H){d->y=-(rand()%(H*4));d->speed=1+rand()%3;d->len=6+rand()%(20+density*2);d->bright=(rand()%5==0);d->col=rand()%W;}
    }
}

/* ══════════════════════════════════════════════════════════
   HUD
══════════════════════════════════════════════════════════ */
static double fps_v=60;static double fps_ts=0;static long fc=0;
static const char*MNAMES[]={"MATRIX","AUDIO","GLITCH","NEURAL","FIRE","WAVE","PLASMA","WARP","CELLS","BRAILLE"};
#define NMODES 10
static void draw_hud(void){
    if(!show_hud)return;
    RGB m=pmi();OGO(H,1);OFG(m.r/2,m.g/2,m.b/2);
    char buf[512];
    snprintf(buf,sizeof(buf)," %-7s│%-8s│S:%d│D:%d│%.0ffps│[1-0]mode [Tab]pal [+/-]spd [[][]]den [p]pause [i]inv [b]bold [s]hud [r]rand [q/^C]quit ",MNAMES[mode],pals[palette].name,spd,density,fps_v);
    buf[W]='\0';ob_str(buf);ORESET;
}

/* ══════════════════════════════════════════════════════════
   INPUT THREAD
══════════════════════════════════════════════════════════ */
static void*input_loop(void*a){
    (void)a;
    while(running){
        char c;
        if(read(0,&c,1)==1){
            if(c=='q'||c=='Q'||c==3){running=0;break;}
            if(c>='1'&&c<='9')mode=c-'1';
            if(c=='0')mode=9;
            if(c=='\t')palette=(palette+1)%NPALS;
            if(c=='+'||c=='='){spd++;if(spd>10)spd=10;}
            if(c=='-'){spd--;if(spd<1)spd=1;}
            if(c==']'){density++;if(density>10)density=10;}
            if(c=='['){density--;if(density<1)density=1;}
            if(c=='s'||c=='S')show_hud=!show_hud;
            if(c=='p'||c=='P')paused=!paused;
            if(c=='i'||c=='I')inverted=!inverted;
            if(c=='b'||c=='B')bold_heads=!bold_heads;
            if(c=='r'||c=='R'){palette=rand()%NPALS;spd=rand()%8+2;density=rand()%8+2;}
        }
        usleep(8000);
    }
    return NULL;
}

/* ══════════════════════════════════════════════════════════
   HELP
══════════════════════════════════════════════════════════ */
static void show_help(void){
    printf(
    "\n"
    "  ██████╗██╗   ██╗██████╗ ███████╗██████╗ ██╗   ██╗██╗███████╗\n"
    "  ██╔════╝╚██╗ ██╔╝██╔══██╗██╔════╝██╔══██╗██║   ██║██║██╔════╝\n"
    "  ██║      ╚████╔╝ ██████╔╝█████╗  ██████╔╝██║   ██║██║███████╗\n"
    "  ██║       ╚██╔╝  ██╔══██╗██╔══╝  ██╔══██╗╚██╗ ██╔╝██║╚════██║\n"
    "  ╚██████╗   ██║   ██████╔╝███████╗██║  ██║ ╚████╔╝ ██║███████║\n"
    "   ╚═════╝   ╚═╝   ╚═════╝ ╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚═╝╚══════╝\n"
    "  v5.0 — Terminal Spectral Engine\n"
    "  Build: gcc -O3 -o cybervis cybervis.c -lm -lpthread\n"
    "\n"
    "  USAGE\n"
    "    cybervis [options]\n"
    "    cybervis help\n"
    "\n"
    "  OPTIONS\n"
    "    -m <mode>     Starting mode (see MODES below)\n"
    "    -c <color>    Starting color palette (see PALETTES below)\n"
    "    -s <1-10>     Speed (default: 5)\n"
    "    -d <1-10>     Density (default: 5)\n"
    "    -f <fps>      FPS cap 10-144 (default: 60)\n"
    "    --no-status   Hide the status bar\n"
    "\n"
    "  MODES\n"
    "    matrix        [1]  Katakana/kanji rain — classic\n"
    "    audio         [2]  FFT spectrum analyzer + waveform\n"
    "    glitch        [3]  Data corruption storm\n"
    "    neural        [4]  Drifting neural network web\n"
    "    fire          [5]  Fluid fire simulation\n"
    "    wave          [6]  Ripple interference patterns\n"
    "    plasma        [7]  Braille plasma field\n"
    "    warp          [8]  Starfield warp drive\n"
    "    cells         [9]  Conway's Game of Life\n"
    "    braille       [0]  Braille character rain\n"
    "\n"
    "  PALETTES (22 total)\n"
    "    green/phosphor  ice/cyan    blood/red   solar/amber\n"
    "    ultra/purple    neon        mono        ember\n"
    "    arctic          sakura      acid        ghost\n"
    "    lava            ocean       gold        toxic\n"
    "    pink            cobalt      copper      mint\n"
    "    crimson         khaki\n"
    "\n"
    "  KEYBINDINGS (live, while running)\n"
    "    1-9, 0      Switch mode\n"
    "    Tab         Next palette\n"
    "    + / -       Speed up / down\n"
    "    ] / [       Density up / down\n"
    "    p           Pause / unpause\n"
    "    i           Invert colors\n"
    "    b           Toggle bright heads\n"
    "    s           Toggle status bar\n"
    "    r           Randomize palette + speed + density\n"
    "    q / Ctrl+C  Quit\n"
    "\n"
    "  EXAMPLES\n"
    "    cybervis\n"
    "    cybervis -m fire -c lava -s 8\n"
    "    cybervis -m warp -c ghost -d 9\n"
    "    cybervis -m plasma -c sakura -f 144\n"
    "    cybervis -m matrix -c blood --no-status\n"
    "\n"
    );
}

/* ══════════════════════════════════════════════════════════
   CLI PARSING
══════════════════════════════════════════════════════════ */
static void parse_args(int argc,char**argv){
    for(int i=1;i<argc;i++){
        /* handle both: cybervis help  AND  cybervis --help  AND  cybervis -h */
        if(!strcmp(argv[i],"help")||!strcmp(argv[i],"--help")||!strcmp(argv[i],"-h")){show_help();exit(0);}
        else if(!strcmp(argv[i],"-m")&&i+1<argc){i++;
            if(!strcmp(argv[i],"matrix"))     mode=0;
            else if(!strcmp(argv[i],"audio")) mode=1;
            else if(!strcmp(argv[i],"glitch"))mode=2;
            else if(!strcmp(argv[i],"neural"))mode=3;
            else if(!strcmp(argv[i],"fire"))  mode=4;
            else if(!strcmp(argv[i],"wave"))  mode=5;
            else if(!strcmp(argv[i],"plasma"))mode=6;
            else if(!strcmp(argv[i],"warp"))  mode=7;
            else if(!strcmp(argv[i],"cells")) mode=8;
            else if(!strcmp(argv[i],"braille"))mode=9;
        } else if(!strcmp(argv[i],"-c")&&i+1<argc){i++;
            const char*cl[]={ "green","phosphor","ice","cyan","blood","red","solar","amber","ultra","purple","neon","mono","ember","arctic","sakura","acid","ghost","lava","ocean","gold","toxic","pink","cobalt","copper","mint","crimson","khaki",NULL};
            const int  ci[]={ 0,0, 1,1, 2,2, 3,3, 4,4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21};
            for(int j=0;cl[j];j++){if(!strcmp(argv[i],cl[j])){palette=ci[j];break;}}
        } else if(!strcmp(argv[i],"-s")&&i+1<argc){spd=atoi(argv[++i]);if(spd<1)spd=1;if(spd>10)spd=10;}
          else if(!strcmp(argv[i],"-d")&&i+1<argc){density=atoi(argv[++i]);if(density<1)density=1;if(density>10)density=10;}
          else if(!strcmp(argv[i],"-f")&&i+1<argc){fps_cap=atoi(argv[++i]);if(fps_cap<10)fps_cap=10;if(fps_cap>144)fps_cap=144;}
          else if(!strcmp(argv[i],"--no-status"))show_hud=0;
    }
}

/* ══════════════════════════════════════════════════════════
   MAIN
══════════════════════════════════════════════════════════ */
int main(int argc,char**argv){
    srand((unsigned)time(NULL));
    parse_args(argc,argv);

    signal(SIGINT,on_signal);signal(SIGTERM,on_signal);signal(SIGWINCH,on_resize);
    get_size();if(W<10||H<5){fprintf(stderr,"Terminal too small.\n");return 1;}
    enable_raw();printf(ALT_ON HIDE_CURSOR);fflush(stdout);OCLEAR;OHOME;ob_flush();

    grid_alloc();
    matrix_init();audio_init();neural_init();fire_init();wave_init();stars_init();ca_init();

    pthread_t inp;pthread_create(&inp,NULL,input_loop,NULL);
    int pm=-1;fps_ts=now_ms();

    while(running){
        double fs=now_ms(),tgt=1000.0/fps_cap;
        if(mode!=pm){
            OCLEAR;OHOME;ob_flush();
            memset(grid,0,H*W*sizeof(Cell));memset(pgrid,0,H*W*sizeof(Cell));ng=0;
            if(mode==0||mode==9)matrix_init();
            if(mode==1)audio_init();
            if(mode==3)neural_init();
            if(mode==4)fire_init();
            if(mode==5)wave_init();
            if(mode==7)stars_init();
            if(mode==8)ca_init();
            pm=mode;
        }
        if(!paused){
            switch(mode){
                case 0:matrix_tick();      grid_render();  break;
                case 1:audio_tick();       audio_render(); break;
                case 2:glitch_tick();      glitch_render();break;
                case 3:neural_tick();      grid_render();  break;
                case 4:fire_tick();        grid_render();  break;
                case 5:wave_tick();        grid_render();  break;
                case 6:plasma_tick();      grid_render();  break;
                case 7:stars_tick();       grid_render();  break;
                case 8:ca_tick();          grid_render();  break;
                case 9:braille_rain_tick();grid_render();  break;
            }
        }
        draw_hud();ob_flush();
        fc++;double n=now_ms();if(fc%30==0){fps_v=30000.0/(n-fps_ts);fps_ts=n;}
        double el=n-fs;if(el<tgt)usleep((int)((tgt-el)*1000));
    }
    pthread_cancel(inp);pthread_join(inp,NULL);cleanup();
    printf("CYBERVIS terminated.\n");return 0;
}
