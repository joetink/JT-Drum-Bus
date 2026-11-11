#include "PluginEditor.h"
// JT Drum Bus - REBUILT TO AGREED MOCKUP 2026-11-11
// Mockup: /mnt/data/gallery/image_20261110_212804.webp
// Chain: Shaper -> G Comp -> D-Sat -> Clip (Clip last)
// FIXES vs previous build (same bugs as 500 Strip / Mix Bus):
//  - C1083: PluginProcessor.h included JuceHeader.h (JUCE 5) - fixed to modular includes in PluginProcessor.h
//  - Meters created AFTER setSize() -> stayed 0,0,0,0 -> invisible. Now bounds set IMMEDIATELY + toFront + resized() at end.
//  - Combo clipping (x=4,12pt -> x=10,11pt, wider boxes), flat panels -> gradient, flat knobs -> 3D knobs
//  - Fake top bar: now REAL BYPASS + REAL IN/OUT VUs driven by DSP inLevelDb/outLevelDb
// Per-module ON toggles (SHAPER_ON / COMP_ON / SAT_ON / CLIP_ON) are the per-module bypass switches.
// Global BYPASS in top bar bypasses everything.
struct KnobDrum : juce::Slider { KnobDrum(): juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox){ setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);} juce::Colour labelColour = juce::Colours::white; juce::String label; };
struct VUMeterDrum : juce::Component {
  float grDb=0; void setGR(float db){ grDb=db; repaint(); }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xfff5e8c8)); g.setColour(juce::Colours::black); g.drawRect(0,0,getWidth(),getHeight(),2);
    g.setColour(juce::Colours::black); g.setFont(12.0f); g.drawText("GAIN REDUCTION",0,4,getWidth(),16,juce::Justification::centred);
    auto cx=getWidth()*0.5f, cy=getHeight()*0.92f, r=getWidth()*0.42f;
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    for(int i=0;i<=10;i++){ float a=juce::jmap((float)i,0.f,10.f,-2.2f,0.6f); g.drawLine(cx+std::cos(a+1.57f)*r, cy+std::sin(a+1.57f)*r, cx+std::cos(a+1.57f)*(r+5), cy+std::sin(a+1.57f)*(r+5), 1); }
    g.setFont(10.0f); g.drawText("0", cx+r*0.85f, cy-18, 20, 12, juce::Justification::centred); g.drawText("-20", cx-r*0.85f-10, cy-18, 24, 12, juce::Justification::centred);
    float pos=juce::jlimit(-20.0f,0.0f,grDb); float ang=juce::jmap(pos,-20.0f,0.0f,-2.2f,0.6f);
    g.setColour(juce::Colours::red); g.drawLine(cx,cy,cx+std::cos(ang+1.57f)*r, cy+std::sin(ang+1.57f)*r,2.5f);
    g.setColour(juce::Colours::black); g.setFont(13.0f); g.drawText(juce::String(grDb,1)+" dB",0,getHeight()-18,getWidth(),16,juce::Justification::centred);
  }
};
struct WaveScopeDrum : juce::Component {
  DrumBus* bus=nullptr; void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xff0a0a0a)); g.setColour(juce::Colour(0xffc65a1e)); g.drawRect(0,0,getWidth(),getHeight(),1);
    if(!bus){ g.setColour(juce::Colours::white); g.drawText("WAVEFORM SCOPE",0,10,getWidth(),20,juce::Justification::centred); return; }
    float ceilLin = std::pow(10, bus->clipCeil/20.0f);
    float cy = getHeight()*0.55f; float ceilPx = ceilLin * getHeight()*0.40f;
    g.setColour(juce::Colours::red.withAlpha(0.7f)); g.setFont(10.0f);
    g.drawLine(0, cy-ceilPx, getWidth(), cy-ceilPx, 1); g.drawLine(0, cy+ceilPx, getWidth(), cy+ceilPx, 1);
    g.drawText("CEIL", 4, cy-ceilPx-14, 40, 12, juce::Justification::left);
    g.setColour(juce::Colours::white.withAlpha(0.3f)); g.drawLine(0, cy, getWidth(), cy, 1);
    g.setColour(juce::Colour(0xffffa500)); juce::Path p; bool st=false;
    for(int i=0;i<DrumBus::scopeN;i++){ int idx=(bus->scopePos+i)%DrumBus::scopeN; float v=bus->scopeBuf[idx]; float x=juce::jmap((float)i,0.f,(float)DrumBus::scopeN,0.f,(float)getWidth()); float y=cy - v*getHeight()*0.40f; if(!st){p.startNewSubPath(x,y); st=true;} else p.lineTo(x,y); }
    g.strokePath(p, juce::PathStrokeType(1.5f));
    g.setColour(juce::Colours::orange); g.setFont(12.0f); g.drawText("CLIP METER  " + juce::String(bus->clipMeterDb,1) + " dB", 6, getHeight()-18, getWidth()-12, 16, juce::Justification::left);
  }
};
struct TopVUD : juce::Component {
  float levelDb=-60; juce::String label="IN";
  void setLevel(float db){ levelDb=db; repaint(); }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xfff5e8c8)); g.setColour(juce::Colours::black); g.drawRect(0,0,getWidth(),getHeight(),1);
    g.setColour(juce::Colours::black); g.setFont(10.0f); g.drawText(label,0,2,getWidth(),12,juce::Justification::centred);
    auto cx=getWidth()*0.5f, cy=(float)getHeight()+4, r=getWidth()*0.42f;
    float pos=juce::jlimit(-60.0f,0.0f,levelDb); float ang=juce::jmap(pos,-60.0f,0.0f,-2.2f,0.6f);
    g.setColour(juce::Colours::red); g.drawLine(cx,cy,cx+std::cos(ang+1.57f)*r, cy+std::sin(ang+1.57f)*r,1.5f);
    g.setColour(juce::Colours::black); g.setFont(9.0f); g.drawText(juce::String(levelDb,1)+" dB",0,getHeight()-12,getWidth(),10,juce::Justification::centred);
  }
};
struct LookDrum : juce::LookAndFeel_V4 {
  void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float, float, juce::Slider& s) override {
    auto cx=x+w*0.5f, cy=y+h*0.5f, r=juce::jmin(w,h)*0.42f;
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    for(int i=0;i<11;i++){ float a=juce::jmap((float)i,0.f,10.f,-2.618f,2.618f); g.drawLine(cx+std::cos(a+1.57f)*(r+3),cy+std::sin(a+1.57f)*(r+3),cx+std::cos(a+1.57f)*(r+7),cy+std::sin(a+1.57f)*(r+7),1); }
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillEllipse(cx-r+2,cy-r+3,r*2,r*2);
    juce::ColourGradient grad(juce::Colour(0xff3a3a3a), cx-r, cy-r, juce::Colour(0xff0a0a0a), cx+r, cy+r, false);
    g.setGradientFill(grad); g.fillEllipse(cx-r,cy-r,r*2,r*2);
    g.setColour(juce::Colour(0xff777777)); g.drawEllipse(cx-r,cy-r,r*2,r*2,1.5f);
    g.setColour(juce::Colours::white.withAlpha(0.18f)); g.fillEllipse(cx-r*0.7f,cy-r*0.7f,r*0.9f,r*0.6f);
    g.setColour(juce::Colour(0xff999999)); g.fillEllipse(cx-r*0.28f,cy-r*0.28f,r*0.56f,r*0.56f);
    float ang=juce::jmap(pos,0.f,1.f,-2.618f,2.618f);
    g.setColour(juce::Colours::white); g.drawLine(cx,cy,cx+std::cos(ang+1.57f)*r*0.82f, cy+std::sin(ang+1.57f)*r*0.82f, 2.5f);
    if(auto* k=dynamic_cast<KnobDrum*>(&s)){ g.setColour(k->labelColour); g.setFont(10.0f); g.drawText(k->label, x-6, y+h-2, w+12, 13, juce::Justification::centred); }
  }
  void drawComboBox(juce::Graphics& g,int w,int h,bool, int,int,int,int, juce::ComboBox& box) override {
    g.setColour(juce::Colour(0xff222222)); g.fillRect(0,0,w,h); g.setColour(juce::Colours::white); g.drawRect(0,0,w,h,1);
    g.setFont(11.0f); g.drawText(box.getText(),10,0,w-30,h,juce::Justification::centredLeft); g.drawText("v",w-20,0,14,h,juce::Justification::centred);
  }
};
static LookDrum lookDrum;
DrumBusEditor::DrumBusEditor(DrumBusAudioProcessor& proc): AudioProcessorEditor(proc), p(proc){
  setSize(1480, 720); setLookAndFeel(&lookDrum);
  titleLabel.setText("JT DRUM BUS - SHAPER - G-SERIES - 500 SERIES", juce::dontSendNotification);
  titleLabel.setFont(juce::Font(24.0f, juce::Font::bold)); titleLabel.setColour(juce::Label::textColourId, juce::Colours::white); addAndMakeVisible(titleLabel);
  // REAL GLOBAL BYPASS - top bar, TextButton + ButtonAttachment to BYPASS
  bypassBtn = std::make_unique<juce::TextButton>("BYPASS"); bypassBtn->setBounds(680, 14, 90, 32); bypassBtn->setClickingTogglesState(true);
  bypassBtn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xffc0272d));
  bypassBtn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff5a0f0f));
  bypassBtn->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  bypassBtn->setColour(juce::TextButton::textColourOnId, juce::Colour(0xffff8888));
  addAndMakeVisible(bypassBtn.get());
  bypassAtt = std::make_unique<BAtt>(p.apvts, "BYPASS", *bypassBtn);
  // REAL IN/OUT VUs
  inVU = std::make_unique<TopVUD>(); inVU->label="IN"; addAndMakeVisible(inVU.get()); inVU->setBounds(800, 8, 140, 44); inVU->toFront(true);
  outVU = std::make_unique<TopVUD>(); outVU->label="OUT"; addAndMakeVisible(outVU.get()); outVU->setBounds(960, 8, 140, 44); outVU->toFront(true);
  // SHAPER - red, white labels - per-module bypass = SHAPER ON toggle
  addToggle("SHAPER_ON","SHAPER ON",20,100,120,22);
  addKnob("SH_ATTACK","ATTACK",20,135,80,80); addKnob("SH_SUSTAIN","SUSTAIN",110,135,80,80); addKnob("SH_SPEED","SPEED",200,135,80,80);
  addKnob("SH_SEP","SEPARATION",20,235,80,80); addKnob("SH_MIX","MIX",110,235,80,80);
  // G COMP - blue, white labels
  addToggle("COMP_ON","G COMP ON",370,100,120,22); addToggle("COMP_AUTOGAIN","AUTO GAIN",500,100,120,22);
  addKnob("COMP_THRESH","THRESH",370,135,80,80); addKnob("COMP_MAKEUP","MAKEUP",460,135,80,80); addKnob("COMP_MIX","MIX",550,135,80,80);
  addChoice("COMP_ATTACK",370,245,96,26); addChoice("COMP_RELEASE",466,245,96,26); addChoice("COMP_RATIO",562,245,96,26);
  // D-SAT - dark, white labels - Drive + Mix only, NO type selector (by design)
  addToggle("SAT_ON","D-SAT ON",710,100,110,22); addToggle("SAT_AUTOGAIN","AUTO GAIN",830,100,120,22);
  addKnob("SAT_DRIVE","DRIVE",710,135,80,80); addKnob("SAT_MIX","MIX",800,135,80,80);
  // CLIPPER - orange, white labels
  addToggle("CLIP_ON","CLIP ON",1070,100,90,22); addToggle("CLIP_AUTOGAIN","AUTO GAIN",1170,100,120,22);
  addKnob("CLIP_CEIL","CEIL",1070,135,80,80); addKnob("CLIP_MIX","MIX",1160,135,80,80);
  addChoice("CLIP_TYPE",1070,245,120,26);
  // Meters - bounds IMMEDIATELY (same bug as 500 Strip / Mix Bus)
  vu = std::make_unique<VUMeterDrum>(); addAndMakeVisible(vu.get()); vu->setBounds(370, 310, 300, 240); vu->toFront(true);
  scope = std::make_unique<WaveScopeDrum>(); scope->bus=&p.bus; addAndMakeVisible(scope.get()); scope->setBounds(1070, 310, 370, 240); scope->toFront(true);
  startTimer(30);
  resized();
}
DrumBusEditor::~DrumBusEditor(){ stopTimer(); setLookAndFeel(nullptr); }
void DrumBusEditor::timerCallback(){ if(vu) vu->setGR(p.bus.compGR); if(scope) scope->repaint(); if(inVU) inVU->setLevel(p.bus.inLevelDb); if(outVU) outVU->setLevel(p.bus.outLevelDb); repaint(); }
juce::Slider* DrumBusEditor::addKnob(const juce::String& id, const juce::String& label, int x,int y,int w,int h){ return addKnob(id,label,x,y,w,h, juce::Colours::white); }
juce::Slider* DrumBusEditor::addKnob(const juce::String& id, const juce::String& label, int x,int y,int w,int h, juce::Colour labelColour){
  auto s=std::make_unique<KnobDrum>(); s->label=label; s->labelColour=labelColour; s->setBounds(x,y,w,h); addAndMakeVisible(s.get());
  auto a=std::make_unique<Att>(p.apvts,id,*s); sliders.push_back(std::move(s)); satts.push_back(std::move(a));
  auto lb=std::make_unique<juce::Label>(); lb->setText(label, juce::dontSendNotification); lb->setFont(juce::Font(11.0f, juce::Font::bold)); lb->setColour(juce::Label::textColourId, labelColour); lb->setJustificationType(juce::Justification::centred); lb->setBounds(x-10,y+h, w+20, 16); addAndMakeVisible(lb.get()); knobLabels.push_back(std::move(lb));
  return sliders.back().get();
}
juce::ToggleButton* DrumBusEditor::addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h){ return addToggle(id,label,x,y,w,h, juce::Colours::white); }
juce::ToggleButton* DrumBusEditor::addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h, juce::Colour textColour){ auto t=std::make_unique<juce::ToggleButton>(label); t->setBounds(x,y,w,h); t->setColour(juce::ToggleButton::textColourId, textColour); t->setColour(juce::ToggleButton::tickColourId, textColour); addAndMakeVisible(t.get()); auto a=std::make_unique<BAtt>(p.apvts,id,*t); toggles.push_back(std::move(t)); batts.push_back(std::move(a)); return toggles.back().get(); }
juce::ComboBox* DrumBusEditor::addChoice(const juce::String& id,int x,int y,int w,int h){
  auto c=std::make_unique<juce::ComboBox>(); c->setBounds(x,y,w,h);
  if(id=="COMP_ATTACK"){ c->addItem("0.1ms",1); c->addItem("0.3ms",2); c->addItem("1ms",3); c->addItem("3ms",4); c->addItem("10ms",5); c->addItem("30ms",6); }
  else if(id=="COMP_RELEASE"){ c->addItem("0.1s",1); c->addItem("0.3s",2); c->addItem("0.6s",3); c->addItem("1.2s",4); c->addItem("AUTO",5); }
  else if(id=="COMP_RATIO"){ c->addItem("2:1",1); c->addItem("4:1",2); c->addItem("10:1",3); }
  else if(id=="CLIP_TYPE"){ c->addItem("SOFT",1); c->addItem("HARD",2); c->addItem("ASYMM",3); }
  addAndMakeVisible(c.get()); auto a=std::make_unique<CAtt>(p.apvts,id,*c); combos.push_back(std::move(c)); catts.push_back(std::move(a)); return combos.back().get();
}
void DrumBusEditor::paint(juce::Graphics& g){
  g.fillAll(juce::Colour(0xff141414));
  // Top bar - brushed steel like Mix Bus mockup
  {
    juce::ColourGradient steel(juce::Colour(0xffd8d8d8), 0, 0, juce::Colour(0xff9a9a9a), 0, 60, false);
    g.setGradientFill(steel); g.fillRect(0,0,1480,60);
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    for(int y=2;y<60;y+=3) g.drawLine(0,y,1480,y,1);
    g.setColour(juce::Colours::black); g.setFont(juce::Font(22.0f, juce::Font::bold)); g.drawText("JT DRUM BUS  -  SHAPER  -  G-SERIES  -  500 SERIES", 20, 8, 600, 30, juce::Justification::left);
    g.setFont(11.0f); g.drawText("SHAPER  ->  G COMP  ->  D-SAT  ->  CLIPPER", 20, 38, 600, 16, juce::Justification::left);
    // BYPASS + IN/OUT are REAL components - NOT drawn here
    g.setColour(juce::Colours::black); g.setFont(juce::Font(20.0f, juce::Font::bold)); g.drawText("JT", 1360, 12, 60, 36, juce::Justification::centred);
  }
  auto panel=[&](int x,int y,int w,int h,juce::Colour col, juce::String title, juce::Colour titleCol){
    juce::ColourGradient grad(col.brighter(0.08f), (float)x, (float)y, col.darker(0.12f), (float)x, (float)(y+h), false);
    g.setGradientFill(grad); g.fillRect(x,y,w,h);
    g.setColour(juce::Colours::white.withAlpha(0.10f)); g.fillRect(x,y,w,3);
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.drawRect(x,y,w,h,2);
    g.setColour(juce::Colour(0xff888888));
    for(auto [sx,sy] : {std::pair<int,int>{x+6,y+6},{x+w-10,y+6},{x+6,y+h-10},{x+w-10,y+h-10}}){ g.fillEllipse(sx,sy,6,6); g.setColour(juce::Colours::black); g.drawLine(sx+1,sy+3,sx+5,sy+3,1); g.setColour(juce::Colour(0xff888888)); }
    g.setColour(titleCol); g.setFont(juce::Font(13.0f, juce::Font::bold)); g.drawText(title, x+10, y+6, w-20, 16, juce::Justification::left);
  };
  panel(10,70,340,620, juce::Colour(0xff8a2a2a), "TRANSIENT SHAPER", juce::Colours::white);
  panel(360,70,320,620, juce::Colour(0xff4a5a6a), "G-SERIES BUS COMP", juce::Colours::white);
  panel(700,70,340,620, juce::Colour(0xff2b2b2b), "D-SATURATION", juce::Colours::white);
  panel(1060,70,400,620, juce::Colour(0xffc65a1e), "CLIPPER + WAVEFORM", juce::Colours::white);
  g.setColour(juce::Colours::white); g.setFont(11.0f);
  g.drawText("ATTACK",370,273,96,14,juce::Justification::centred); g.drawText("RELEASE",466,273,96,14,juce::Justification::centred); g.drawText("RATIO",562,273,96,14,juce::Justification::centred);
  g.drawText("TYPE",1070,273,120,14,juce::Justification::centred);
  g.drawText("GAIN REDUCTION", 370, 290, 300, 14, juce::Justification::centred);
  g.drawText("WAVEFORM", 1070, 290, 370, 14, juce::Justification::centred);
  g.drawText("DRIVE + MIX ONLY - NO TYPE", 710, 300, 320, 14, juce::Justification::centred);
  // Shaper graph - Attack / Sustain shape (mockup)
  {
    int ex=20, ey=340, ew=320, eh=160;
    g.setColour(juce::Colours::black); g.fillRect(ex,ey,ew,eh);
    g.setColour(juce::Colours::white.withAlpha(0.15f)); g.drawLine(ex, ey+eh/2, ex+ew, ey+eh/2, 1);
    g.setColour(juce::Colours::white); g.setFont(11.0f); g.drawText("SHAPER", ex+8, ey+4, 100, 14, juce::Justification::left);
    // Draw transient shape: attack spike + sustain tail, scaled by SH_ATTACK / SH_SUSTAIN
    juce::Path path; bool st=false;
    for(int px=0; px<ew; px++){
      float frac=(float)px/(float)ew;
      float env = std::exp(-frac*6.0f) * (1.0f + p.bus.shAttack/100.0f*1.5f) + 0.2f*std::exp(-frac*2.0f)*(1.0f + p.bus.shSustain/100.0f);
      float y = ey+eh-10 - juce::jlimit(0.0f,1.0f,env)* (eh-30);
      float x=(float)(ex+px);
      if(!st){ path.startNewSubPath(x,y); st=true; } else path.lineTo(x,y);
    }
    g.setColour(juce::Colour(0xffff6666)); g.strokePath(path, juce::PathStrokeType(1.8f));
  }
  // D-Sat saturation bar
  {
    int ex=710, ey=340, ew=320, eh=80;
    g.setColour(juce::Colours::black); g.fillRect(ex,ey,ew,eh);
    g.setColour(juce::Colours::white); g.setFont(11.0f); g.drawText("SATURATION", ex+8, ey+4, 100, 14, juce::Justification::left);
    float f=juce::jlimit(0.0f,1.0f,p.bus.satDrive/100.0f);
    g.setColour(juce::Colour(0xffffa500)); g.fillRect(ex+8, ey+30, (int)((ew-16)*f), 18);
    g.setColour(juce::Colours::white); g.drawText(juce::String((int)p.bus.satDrive)+"%", ex+8, ey+52, ew-16, 14, juce::Justification::left);
  }
}
void DrumBusEditor::resized(){
  titleLabel.setBounds(20,10,700,30);
  if(bypassBtn) bypassBtn->setBounds(680, 14, 90, 32);
  if(inVU) inVU->setBounds(800, 8, 140, 44);
  if(outVU) outVU->setBounds(960, 8, 140, 44);
  if(vu) vu->setBounds(370, 310, 300, 240);
  if(scope) scope->setBounds(1070, 310, 370, 240);
}
juce::AudioProcessorEditor* DrumBusAudioProcessor::createEditor(){ return new DrumBusEditor(*this); }
