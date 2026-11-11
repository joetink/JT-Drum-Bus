#include "PluginEditor.h"
// JT Drum Bus - REBUILT FROM SCRATCH TO MOCKUP 2026-11-11
// Same bugs as Mix Bus / 500 Strip screenshots: title double-drawn, VU needle as red diagonal slash
// (cos(a+1.57), cy=height+4), ComboBoxes clipped / struck through, flat panels, meter bounds 0,0,0,0.
// Fixes, same as Mix Bus TRUE / 500 Strip TRUE:
//  1. No titleLabel. Paint draws top-bar title ONCE. Panel titles drawn ONCE in paint.
//  2. Needle math everywhere: x=cx+sin(a)*r, y=cy-cos(a)*r, pivot bottom-centre, a -0.9..+0.9 rad.
//  3. drawComboBox: text at x=8, width w-28, arrow at w-20, font 11, boxes 96-120 wide.
//  4. Knob labels: Label ONLY, LookAndFeel draws no text.
//  5. Meters bounds set immediately + resized() at end. No 0,0,0,0 invisible meters.
// Chain: Shaper -> G Comp -> D-Sat -> Clipper (Clip last). D-Sat: Drive + Mix only, NO type selector.
struct KnobDrum : juce::Slider { KnobDrum(): juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox){ setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);} };
struct SmallVUD : juce::Component {
  float db=-60; juce::String label="IN"; void setDb(float v){ db=v; repaint(); }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xfff5e8c8)); g.setColour(juce::Colours::black); g.drawRect(0,0,getWidth(),getHeight(),1);
    g.setColour(juce::Colours::black); g.setFont(9.0f); g.drawText(label,0,2,getWidth(),10,juce::Justification::centred);
    float cx=getWidth()*0.5f, cy=(float)getHeight()-4, r=getWidth()*0.42f;
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    for(int i=0;i<=5;i++){ float a=juce::jmap((float)i,0.f,5.f,-0.9f,0.9f); g.drawLine(cx+std::sin(a)*r,cy-std::cos(a)*r,cx+std::sin(a)*(r+3),cy-std::cos(a)*(r+3),1); }
    float pos=juce::jlimit(-60.0f,3.0f,db); float ang=juce::jmap(pos,-60.0f,3.0f,-0.9f,0.9f);
    g.setColour(juce::Colours::red); g.drawLine(cx,cy,cx+std::sin(ang)*r,cy-std::cos(ang)*r,1.5f);
    g.setColour(juce::Colours::black); g.setFont(8.0f); g.drawText(juce::String(db,1)+" dB",0,getHeight()-11,getWidth(),10,juce::Justification::centred);
  }
};
struct GRVUD : juce::Component {
  float grDb=0; void setGR(float v){ grDb=v; repaint(); }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xfff5e8c8)); g.setColour(juce::Colours::black); g.drawRect(0,0,getWidth(),getHeight(),1);
    g.setColour(juce::Colours::black); g.setFont(10.0f); g.drawText("GAIN REDUCTION",0,4,getWidth(),12,juce::Justification::centred);
    float cx=getWidth()*0.5f, cy=(float)getHeight()-6, r=getWidth()*0.42f;
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    for(int i=0;i<=10;i++){ float a=juce::jmap((float)i,0.f,10.f,-0.9f,0.9f); g.drawLine(cx+std::sin(a)*r,cy-std::cos(a)*r,cx+std::sin(a)*(r+4),cy-std::cos(a)*(r+4),1); }
    g.setFont(9.0f); g.drawText("0",cx+r*0.75f,cy-16,18,10,juce::Justification::centred); g.drawText("-20",cx-r*0.75f-8,cy-16,22,10,juce::Justification::centred);
    float pos=juce::jlimit(-20.0f,0.0f,grDb); float ang=juce::jmap(pos,-20.0f,0.0f,-0.9f,0.9f);
    g.setColour(juce::Colours::red); g.drawLine(cx,cy,cx+std::sin(ang)*r,cy-std::cos(ang)*r,2.0f);
    g.setColour(juce::Colours::black); g.setFont(11.0f); g.drawText(juce::String(grDb,1)+" dB",0,getHeight()-16,getWidth(),14,juce::Justification::centred);
  }
};
struct WaveScopeD : juce::Component {
  DrumBus* bus=nullptr; void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xff0a0a0a)); g.setColour(juce::Colour(0xffc65a1e)); g.drawRect(0,0,getWidth(),getHeight(),1);
    if(!bus) return;
    float ceilLin=std::pow(10,bus->clipCeil/20.0f); float cy=getHeight()*0.55f; float ceilPx=ceilLin*getHeight()*0.40f;
    g.setColour(juce::Colours::red.withAlpha(0.7f)); g.setFont(10.0f);
    g.drawLine(0,cy-ceilPx,getWidth(),cy-ceilPx,1); g.drawLine(0,cy+ceilPx,getWidth(),cy+ceilPx,1);
    g.drawText("CEIL",4,cy-ceilPx-14,40,12,juce::Justification::left);
    g.setColour(juce::Colours::white.withAlpha(0.3f)); g.drawLine(0,cy,getWidth(),cy,1);
    g.setColour(juce::Colour(0xffffa500)); juce::Path p; bool st=false;
    for(int i=0;i<DrumBus::scopeN;i++){ int idx=(bus->scopePos+i)%DrumBus::scopeN; float v=bus->scopeBuf[idx]; float x=juce::jmap((float)i,0.f,(float)DrumBus::scopeN,0.f,(float)getWidth()); float y=cy-v*getHeight()*0.40f; if(!st){p.startNewSubPath(x,y); st=true;} else p.lineTo(x,y); }
    g.strokePath(p, juce::PathStrokeType(1.5f));
    g.setColour(juce::Colours::orange); g.setFont(12.0f); g.drawText("CLIP METER  "+juce::String(bus->clipMeterDb,1)+" dB",6,getHeight()-18,getWidth()-12,16,juce::Justification::left);
  }
};
struct LookDrumT : juce::LookAndFeel_V4 {
  void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float, float, juce::Slider&) override {
    float cx=x+w*0.5f, cy=y+h*0.5f, r=juce::jmin(w,h)*0.42f;
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    for(int i=0;i<11;i++){ float a=juce::jmap((float)i,0.f,10.f,-2.356f,2.356f); g.drawLine(cx+std::sin(a)*(r+3),cy-std::cos(a)*(r+3),cx+std::sin(a)*(r+7),cy-std::cos(a)*(r+7),1); }
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillEllipse(cx-r+2,cy-r+3,r*2,r*2);
    juce::ColourGradient grad(juce::Colour(0xff3a3a3a),cx-r,cy-r,juce::Colour(0xff0a0a0a),cx+r,cy+r,false);
    g.setGradientFill(grad); g.fillEllipse(cx-r,cy-r,r*2,r*2);
    g.setColour(juce::Colour(0xff777777)); g.drawEllipse(cx-r,cy-r,r*2,r*2,1.5f);
    g.setColour(juce::Colours::white.withAlpha(0.18f)); g.fillEllipse(cx-r*0.7f,cy-r*0.7f,r*0.9f,r*0.6f);
    float ang=juce::jmap(pos,0.f,1.f,-2.356f,2.356f);
    g.setColour(juce::Colours::white); g.drawLine(cx,cy,cx+std::sin(ang)*r*0.82f,cy-std::cos(ang)*r*0.82f,2.5f);
    // NO label here - Label component below knob draws it, once.
  }
  void drawComboBox(juce::Graphics& g,int w,int h,bool, int,int,int,int, juce::ComboBox& box) override {
    g.setColour(juce::Colour(0xff222222)); g.fillRect(0,0,w,h); g.setColour(juce::Colours::white); g.drawRect(0,0,w,h,1);
    g.setFont(11.0f); g.drawText(box.getText(),8,0,w-28,h,juce::Justification::centredLeft); g.drawText("v",w-20,0,14,h,juce::Justification::centred);
  }
  void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool, bool) override {
    if(b.getName()=="BYPASS"){ bool on=b.getToggleState(); g.setColour(on?juce::Colour(0xff5a0f0f):juce::Colour(0xffc0272d)); g.fillRect(0,0,b.getWidth(),b.getHeight()); g.setColour(juce::Colours::white); g.drawRect(0,0,b.getWidth(),b.getHeight(),1); g.setFont(juce::Font(12.0f,juce::Font::bold)); g.drawText("BYPASS",0,0,b.getWidth(),b.getHeight(),juce::Justification::centred); return; }
    LookAndFeel_V4::drawButtonBackground(g,b,juce::Colours::grey,false,false);
  }
};
static LookDrumT lookDrumT;
DrumBusEditor::DrumBusEditor(DrumBusAudioProcessor& proc): AudioProcessorEditor(proc), p(proc){
  setSize(1480, 680); setLookAndFeel(&lookDrumT);
  // REAL GLOBAL BYPASS - red rect top bar, ButtonAttachment to BYPASS. Per-module ON toggles are the per-module bypasses.
  bypassBtn = std::make_unique<juce::TextButton>("BYPASS"); bypassBtn->setName("BYPASS"); bypassBtn->setBounds(680,14,110,32); bypassBtn->setClickingTogglesState(true); addAndMakeVisible(bypassBtn.get());
  bypassAtt = std::make_unique<BAtt>(p.apvts,"BYPASS",*bypassBtn);
  inVU = std::make_unique<SmallVUD>(); inVU->label="IN"; addAndMakeVisible(inVU.get()); inVU->setBounds(820,10,110,50);
  outVU = std::make_unique<SmallVUD>(); outVU->label="OUT"; addAndMakeVisible(outVU.get()); outVU->setBounds(950,10,110,50);
  // SHAPER - red
  addToggle("SHAPER_ON","SHAPER ON",20,102,110,18);
  addKnob("SH_ATTACK","ATTACK",20,130,80,80); addKnob("SH_SUSTAIN","SUSTAIN",110,130,80,80); addKnob("SH_SPEED","SPEED",200,130,80,80);
  addKnob("SH_SEP","SEPARATION",20,230,80,80); addKnob("SH_MIX","MIX",110,230,80,80);
  // G COMP - blue
  addToggle("COMP_ON","G COMP ON",370,102,110,18); addToggle("COMP_AUTOGAIN","AUTO GAIN",490,102,110,18);
  addKnob("COMP_THRESH","THRESH",370,130,80,80); addKnob("COMP_MAKEUP","MAKEUP",460,130,80,80); addKnob("COMP_MIX","MIX",550,130,80,80);
  addChoice("COMP_ATTACK",370,230,96,24); addChoice("COMP_RELEASE",476,230,96,24); addChoice("COMP_RATIO",582,230,96,24);
  vu = std::make_unique<GRVUD>(); addAndMakeVisible(vu.get()); vu->setBounds(370,280,300,130);
  // D-SAT - dark, Drive + Mix only, NO type selector
  addToggle("SAT_ON","D-SAT ON",710,102,100,18); addToggle("SAT_AUTOGAIN","AUTO GAIN",820,102,110,18);
  addKnob("SAT_DRIVE","DRIVE",710,130,80,80); addKnob("SAT_MIX","MIX",800,130,80,80);
  // CLIPPER - orange
  addToggle("CLIP_ON","CLIP ON",1070,102,90,18); addToggle("CLIP_AUTOGAIN","AUTO GAIN",1170,102,110,18);
  addKnob("CLIP_CEIL","CEIL",1070,130,80,80); addKnob("CLIP_MIX","MIX",1160,130,80,80);
  addChoice("CLIP_TYPE",1070,230,120,24);
  scope = std::make_unique<WaveScopeD>(); scope->bus=&p.bus; addAndMakeVisible(scope.get()); scope->setBounds(1070,280,370,130);
  startTimer(30);
  resized();
}
DrumBusEditor::~DrumBusEditor(){ stopTimer(); setLookAndFeel(nullptr); }
void DrumBusEditor::timerCallback(){ if(vu) vu->setGR(p.bus.compGR); if(scope) scope->repaint(); if(inVU) inVU->setDb(p.bus.inLevelDb); if(outVU) outVU->setDb(p.bus.outLevelDb); }
juce::Slider* DrumBusEditor::addKnob(const juce::String& id,const juce::String& label,int x,int y,int w,int h){ return addKnob(id,label,x,y,w,h,juce::Colours::white); }
juce::Slider* DrumBusEditor::addKnob(const juce::String& id,const juce::String& label,int x,int y,int w,int h,juce::Colour lc){
  auto s=std::make_unique<KnobDrum>(); s->setBounds(x,y,w,h); addAndMakeVisible(s.get());
  auto a=std::make_unique<Att>(p.apvts,id,*s); sliders.push_back(std::move(s)); satts.push_back(std::move(a));
  auto lb=std::make_unique<juce::Label>(); lb->setText(label,juce::dontSendNotification); lb->setFont(juce::Font(11.0f,juce::Font::bold)); lb->setColour(juce::Label::textColourId,lc); lb->setJustificationType(juce::Justification::centred); lb->setBounds(x-10,y+h,w+20,16); addAndMakeVisible(lb.get()); knobLabels.push_back(std::move(lb));
  return sliders.back().get();
}
juce::ToggleButton* DrumBusEditor::addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h){ return addToggle(id,label,x,y,w,h,juce::Colours::white); }
juce::ToggleButton* DrumBusEditor::addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h,juce::Colour tc){ auto t=std::make_unique<juce::ToggleButton>(label); t->setBounds(x,y,w,h); t->setColour(juce::ToggleButton::textColourId,tc); t->setColour(juce::ToggleButton::tickColourId,tc); addAndMakeVisible(t.get()); auto a=std::make_unique<BAtt>(p.apvts,id,*t); toggles.push_back(std::move(t)); batts.push_back(std::move(a)); return toggles.back().get(); }
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
  // Top bar - dark steel, title drawn ONCE (no titleLabel)
  {
    juce::ColourGradient steel(juce::Colour(0xff4a4a4a),0,0,juce::Colour(0xff1a1a1a),0,70,false);
    g.setGradientFill(steel); g.fillRect(0,0,1480,70);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); for(int y=2;y<70;y+=3) g.drawLine(0,y,1480,y,1);
    g.setColour(juce::Colour(0xffdddddd)); g.setFont(juce::Font(22.0f,juce::Font::bold)); g.drawText("JT DRUM BUS  -  SHAPER  -  G-SERIES  -  500 SERIES",20,12,620,30,juce::Justification::left);
    g.setFont(11.0f); g.drawText("SHAPER  ->  G COMP  ->  D-SAT  ->  CLIPPER",20,40,620,16,juce::Justification::left);
    g.setColour(juce::Colour(0xffdddddd)); g.setFont(juce::Font(18.0f,juce::Font::bold)); g.drawText("JT",1360,16,60,30,juce::Justification::centred);
  }
  auto panel=[&](int x,int y,int w,int h,juce::Colour col,juce::String title,juce::Colour tc){
    juce::ColourGradient grad(col.brighter(0.08f),(float)x,(float)y,col.darker(0.12f),(float)x,(float)(y+h),false);
    g.setGradientFill(grad); g.fillRect(x,y,w,h);
    g.setColour(juce::Colours::white.withAlpha(0.10f)); g.fillRect(x,y,w,3);
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.drawRect(x,y,w,h,2);
    g.setColour(juce::Colour(0xff888888)); for(auto [sx,sy] : {std::pair<int,int>{x+6,y+6},{x+w-10,y+6},{x+6,y+h-10},{x+w-10,y+h-10}}){ g.fillEllipse(sx,sy,6,6); }
    g.setColour(tc); g.setFont(juce::Font(12.0f,juce::Font::bold)); g.drawText(title,x+10,y+6,w-20,16,juce::Justification::left);
  };
  panel(10,80,340,580,juce::Colour(0xff8a2a2a),"TRANSIENT SHAPER",juce::Colours::white);
  panel(360,80,320,580,juce::Colour(0xff4a5a6a),"G-SERIES BUS COMP",juce::Colours::white);
  panel(700,80,340,580,juce::Colour(0xff2b2b2b),"D-SATURATION",juce::Colours::white);
  panel(1060,80,400,580,juce::Colour(0xffc65a1e),"CLIPPER + WAVEFORM",juce::Colours::white);
  g.setColour(juce::Colours::white); g.setFont(11.0f);
  g.drawText("ATTACK",370,256,96,14,juce::Justification::centred); g.drawText("RELEASE",476,256,96,14,juce::Justification::centred); g.drawText("RATIO",582,256,96,14,juce::Justification::centred);
  g.drawText("TYPE",1070,256,120,14,juce::Justification::centred);
  g.drawText("GAIN REDUCTION",370,262,300,14,juce::Justification::centred);
  g.drawText("WAVEFORM",1070,262,370,14,juce::Justification::centred);
  g.drawText("DRIVE + MIX ONLY - NO TYPE",710,230,320,14,juce::Justification::centred);
  // Shaper graph
  {
    int ex=20, ey=340, ew=320, eh=160;
    g.setColour(juce::Colours::black); g.fillRect(ex,ey,ew,eh);
    g.setColour(juce::Colours::white.withAlpha(0.15f)); g.drawLine(ex,ey+eh/2,ex+ew,ey+eh/2,1);
    g.setColour(juce::Colours::white); g.setFont(11.0f); g.drawText("SHAPER",ex+8,ey+4,100,14,juce::Justification::left);
    juce::Path path; bool st=false;
    for(int px=0; px<ew; px++){ float frac=(float)px/(float)ew; float env=std::exp(-frac*6.0f)*(1.0f+p.bus.shAttack/100.0f*1.5f)+0.2f*std::exp(-frac*2.0f)*(1.0f+p.bus.shSustain/100.0f); float y=ey+eh-10-juce::jlimit(0.0f,1.0f,env)*(eh-30); float x=(float)(ex+px); if(!st){path.startNewSubPath(x,y); st=true;} else path.lineTo(x,y); }
    g.setColour(juce::Colour(0xffff6666)); g.strokePath(path,juce::PathStrokeType(1.8f));
  }
  // D-Sat bar
  {
    int ex=710, ey=340, ew=320, eh=80; g.setColour(juce::Colours::black); g.fillRect(ex,ey,ew,eh);
    g.setColour(juce::Colours::white); g.setFont(11.0f); g.drawText("SATURATION",ex+8,ey+4,100,14,juce::Justification::left);
    float f=juce::jlimit(0.0f,1.0f,p.bus.satDrive/100.0f); g.setColour(juce::Colour(0xffffa500)); g.fillRect(ex+8,ey+30,(int)((ew-16)*f),18);
    g.setColour(juce::Colours::white); g.drawText(juce::String((int)p.bus.satDrive)+"%",ex+8,ey+52,ew-16,14,juce::Justification::left);
  }
}
void DrumBusEditor::resized(){
  if(bypassBtn) bypassBtn->setBounds(680,14,110,32);
  if(inVU) inVU->setBounds(820,10,110,50);
  if(outVU) outVU->setBounds(950,10,110,50);
  if(vu) vu->setBounds(370,280,300,130);
  if(scope) scope->setBounds(1070,280,370,130);
}
juce::AudioProcessorEditor* DrumBusAudioProcessor::createEditor(){ return new DrumBusEditor(*this); }
