#pragma once
#include <cmath>
#include <algorithm>
constexpr double JT_PI_DRUM = 3.14159265358979323846;
struct BiquadD { float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0; void set(float _b0,float _b1,float _b2,float _a1,float _a2){b0=_b0;b1=_b1;b2=_b2;a1=_a1;a2=_a2;} float process(float x){ float y=b0*x+z1; z1=b1*x - a1*y + z2; z2=b2*x - a2*y; return y; } void reset(){z1=z2=0;} };
static float tanhPadeD(float x){ float x2=x*x; return x*(27+x2)/(27+9*x2); }
class DrumBus {
public:
  double sr=48000;
  bool bypass=false; // REAL BYPASS - when true, all processing skipped, dry audio passes through
  float inLevelDb=-60, outLevelDb=-60; // REAL IN/OUT meters for top bar
  // G COMP - CORRECT SSL: feedback, dominant peak, ratio-dependent knee
  bool compOn=true; float compThresh=-10, compMakeup=0, compMix=100; int compAttackIdx=4, compReleaseIdx=4, compRatioIdx=1; float compGain=1, compGR=0; float compPeakEnv=0; bool compAutoGain=true;
  // TRANSIENT SHAPER - Kilohearts style: Attack / Sustain / Speed / Separation / Mix
  bool shaperOn=true; float shAttack=0, shSustain=0, shSpeed=50, shSep=50, shMix=100;
  float shEnv[2]={0,0}; float shEnvSlow[2]={0,0};
  // D-SATURATION - gentle, Drive + Mix only, no type selector, ~150% drive range
  bool satOn=true; float satDrive=30, satMix=100; bool satAutoGain=true;
  // CLIPPER
  bool clipOn=true; float clipCeil=-1, clipMix=100; int clipType=0; bool clipAutoGain=false; float clipMeterDb=0; bool isRendering=false; // 4x oversample on render only
  static constexpr int scopeN=512; float scopeBuf[scopeN]={}; int scopePos=0;
  void prepare(double s){ sr=s; reset(); }
  void reset(){ compGain=1; compGR=0; compPeakEnv=0; shEnv[0]=shEnv[1]=0; shEnvSlow[0]=shEnvSlow[1]=0; clipMeterDb=0; }
  void processGCompStereo(float& L, float& R){
    if(!compOn) return;
    float postL = L * compGain, postR = R * compGain;
    float rect = std::max(fabsf(postL), fabsf(postR));
    float attMs = compAttackIdx==0?0.1f:compAttackIdx==1?0.3f:compAttackIdx==2?1.0f:compAttackIdx==3?3.0f:compAttackIdx==4?10.0f:30.0f;
    float attC = 1-expf(-1.0f/(float)(sr*attMs/1000.0f));
    float relMs = 300;
    if(compReleaseIdx==0) relMs=100; else if(compReleaseIdx==1) relMs=300; else if(compReleaseIdx==2) relMs=600; else if(compReleaseIdx==3) relMs=1200;
    else { if(compGR < -6) relMs=1200; else if(compGR < -2) relMs=600; else relMs=200; }
    float relC = 1-expf(-1.0f/(float)(sr*relMs/1000.0f));
    if(rect > compPeakEnv) compPeakEnv += attC*(rect-compPeakEnv); else compPeakEnv += relC*(rect-compPeakEnv);
    float envDb = 20*log10f(compPeakEnv+1e-12f);
    float ratio = compRatioIdx==0?2:compRatioIdx==1?4:10;
    float knee = ratio==2?12:ratio==4?6:2;
    float threshShift = ratio==2?-4:ratio==4?-2:0;
    float effThresh = compThresh + threshShift;
    float over = envDb - effThresh;
    float grDb = 0;
    if(over > -knee/2){ if(over < knee/2) grDb = (1-1/ratio)*(over+knee/2)*(over+knee/2)/(2*knee); else grDb = (1-1/ratio)*over; }
    float targetGain = powf(10, -grDb/20);
    float coeff = targetGain < compGain ? attC : relC;
    compGain += coeff*(targetGain-compGain);
    compGR = 20*log10f(compGain+1e-12f);
    float makeupLin = powf(10, compMakeup/20);
    float autoMakeup = compAutoGain ? powf(10, -compGR/20*0.5f) : 1.0f;
    float mix = compMix/100;
    L = L*(1-mix) + (L*compGain*makeupLin*autoMakeup)*mix;
    R = R*(1-mix) + (R*compGain*makeupLin*autoMakeup)*mix;
  }
  float processShaper(float x, int ch){
    if(!shaperOn) return x;
    float rect = fabsf(x);
    // Speed: envelope speed - 0 = slow (50ms), 100 = fast (1ms)
    float speedMs = 50 - (shSpeed/100)*49; // 50ms -> 1ms
    float attC = 1-expf(-1.0f/(float)(sr*speedMs/1000.0f));
    float relC = 1-expf(-1.0f/(float)(sr*80.0f/1000.0f));
    if(rect > shEnv[ch]) shEnv[ch] += attC*(rect-shEnv[ch]); else shEnv[ch] += relC*(rect-shEnv[ch]);
    // Slow env for separation
    float slowC = 1-expf(-1.0f/(float)(sr*150.0f/1000.0f));
    shEnvSlow[ch] += slowC*(rect-shEnvSlow[ch]);
    // Separation: 0 = no separation (whole signal), 100 = only transient
    // transient = fast env - slow env, sustain = slow env
    float transient = std::max(0.0f, shEnv[ch] - shEnvSlow[ch]);
    float sustain = shEnvSlow[ch];
    float sep = shSep/100;
    float transAmt = shAttack/100*2.0f; // -100..+100 -> -2..+2 gain
    float sustAmt = shSustain/100*2.0f;
    // Apply: transient part gets Attack gain, sustain part gets Sustain gain, blended by Separation
    float transGain = 1 + transAmt * (transient/(transient+sustain+1e-9f)) * (0.5f+sep*0.5f);
    float sustGain = 1 + sustAmt * (sustain/(transient+sustain+1e-9f)) * (1-sep*0.3f);
    float y = x * (0.5f*(transGain+sustGain));
    float mix = shMix/100; return x*(1-mix)+y*mix;
  }
  float processDSatSingle(float x){ float drive = satDrive/100*1.5f; float y = tanhPadeD(x*(1+drive*0.8f)) * (1/(1+drive*0.2f)); y = y + 0.10f*tanhPadeD(x*drive)*0.3f; return y; }
  float processDSat(float x, int ch){
    if(!satOn) return x;
    // D-Saturation: gentle diode, 150% drive range (0-150 -> 0-1.5x)
    float y; if(isRendering){ float acc=0; float prev=x; for(int i=0;i<4;i++){ float up=prev+(x-prev)*(i/4.0f); acc+=processDSatSingle(up); prev=up; } y=acc/4.0f; } else { y=processDSatSingle(x); }
    float mix = satMix/100; return x*(1-mix)+y*mix;
  }
  float processClipSingle(float x){ float ceilLin = powf(10, clipCeil/20); float y=x; if(clipType==0) y=tanhPadeD(y/ceilLin)*ceilLin; else if(clipType==1) y=std::max(-ceilLin,std::min(ceilLin,y)); else { float pos=ceilLin, neg=ceilLin*0.6f; y=y>0?std::min(y,pos):std::max(y,-neg); y=tanhPadeD(y/ceilLin)*ceilLin; } return y; }
  float processClip(float x, int ch){
    if(!clipOn){ clipMeterDb*=0.995f; return x; }
    float pre=x; float y; if(isRendering){ float acc=0; float prev=x; for(int i=0;i<4;i++){ float up=prev+(x-prev)*(i/4.0f); acc+=processClipSingle(up); prev=up; } y=acc/4.0f; } else { y=processClipSingle(x); }
    float clipDb=0; float aPre=fabsf(pre), aPost=fabsf(y);
    if(aPre>aPost+0.0001f && aPre>0.0001f){ clipDb=20*log10f(aPre/(aPost+1e-12f)); if(clipDb<0) clipDb=0; if(clipDb>24) clipDb=24; }
    if(ch==0) clipMeterDb = clipMeterDb*0.92f + clipDb*0.08f;
    float mix=clipMix/100; y=x*(1-mix)+y*mix;
    if(ch==0){ scopeBuf[scopePos]=y; scopePos=(scopePos+1)%scopeN; }
    return y;
  }
  void processStereo(float& L, float& R){
    float inPeak = std::max(fabsf(L), fabsf(R));
    inLevelDb = 0.9f*inLevelDb + 0.1f*(20*log10f(inPeak+1e-12f));
    if(bypass){
      // REAL BYPASS: no Shaper / Comp / Sat / Clip - dry, GR -> 0, Clip decays, scope shows dry
      compGR=0; clipMeterDb*=0.95f;
      scopeBuf[scopePos]=L; scopePos=(scopePos+1)%scopeN;
      outLevelDb = inLevelDb;
      return;
    }
    // Order: Shaper -> Comp -> Sat -> Clip (Comp after Shaper so it doesn't undo attack boost, Clip last)
    L=processShaper(L,0); R=processShaper(R,1);
    processGCompStereo(L,R);
    L=processDSat(L,0); R=processDSat(R,1);
    L=processClip(L,0); R=processClip(R,1);
    float outPeak = std::max(fabsf(L), fabsf(R));
    outLevelDb = 0.9f*outLevelDb + 0.1f*(20*log10f(outPeak+1e-12f));
  }
};
