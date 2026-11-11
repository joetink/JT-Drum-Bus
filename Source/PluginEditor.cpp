#include "PluginEditor.h"
struct VUMeterDrum : juce::Component {
  float grDb=0; void setGR(float db){ grDb=db; repaint(); }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xfff5e8c8)); g.setColour(juce::Colours::black); g.drawRect(0,0,getWidth(),getHeight(),2);
    g.setColour(juce::Colours::black); g.setFont(13.0f); g.drawText("GAIN REDUCTION",0,4,getWidth(),16,juce::Justification::centred);
    auto cx=getWidth()*0.5f, cy=getHeight()*0.95f, r=getWidth()*0.40f;
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
    g.setColour(juce::Colour(0xffffa500));
    juce::Path p; bool st=false;
    for(int i=0;i<DrumBus::scopeN;i++){ int idx=(bus->scopePos+i)%DrumBus::scopeN; float v=bus->scopeBuf[idx]; float x=juce::jmap((float)i,0.f,(float)DrumBus::scopeN,0.f,(float)getWidth()); float y=cy - v*getHeight()*0.40f; if(!st){p.startNewSubPath(x,y); st=true;} else p.lineTo(x,y); }
    g.strokePath(p, juce::PathStrokeType(1.5f));
    g.setColour(juce::Colours::orange); g.setFont(12.0f); g.drawText("CLIP METER  " + juce::String(bus->clipMeterDb,1) + " dB", 6, getHeight()-18, getWidth()-12, 16, juce::Justification::left);
  }
};
struct LookDrum : juce::LookAndFeel_V4 {
  void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float, float, juce::Slider&) override {
    auto cx=x+w*0.5f, cy=y+h*0.5f, r=juce::jmin(w,h)*0.42f;
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    for(int i=0;i<11;i++){ float a=juce::jmap((float)i,0.f,10.f,-2.618f,2.618f); g.drawLine(cx+std::cos(a+1.57f)*(r+3),cy+std::sin(a+1.57f)*(r+3),cx+std::cos(a+1.57f)*(r+7),cy+std::sin(a+1.57f)*(r+7),1); }
    g.setColour(juce::Colour(0xff1a1a1a)); g.fillEllipse(cx-r,cy-r,r*2,r*2);
    g.setColour(juce::Colour(0xff666666)); g.drawEllipse(cx-r,cy-r,r*2,r*2,1.5f);
    g.setColour(juce::Colour(0xff999999)); g.fillEllipse(cx-r*0.28f,cy-r*0.28f,r*0.56f,r*0.56f);
    float ang=juce::jmap(pos,0.f,1.f,-2.618f,2.618f);
    g.setColour(juce::Colours::white); g.drawLine(cx,cy,cx+std::cos(ang+1.57f)*r*0.8f, cy+std::sin(ang+1.57f)*r*0.8f, 2.5f);
  }
  void drawComboBox(juce::Graphics& g,int w,int h,bool, int,int,int,int, juce::ComboBox& box) override {
    g.setColour(juce::Colour(0xff222222)); g.fillRect(0,0,w,h); g.setColour(juce::Colours::white); g.drawRect(0,0,w,h,1);
    g.setFont(12.0f); g.drawText(box.getText(),4,0,w-18,h,juce::Justification::centredLeft); g.drawText("v",w-18,0,14,h,juce::Justification::centred);
  }
};
static LookDrum lookDrum;
DrumBusEditor::DrumBusEditor(DrumBusAudioProcessor& proc): AudioProcessorEditor(proc), p(proc){
  setSize(1480, 680); setLookAndFeel(&lookDrum);
  titleLabel.setText("JT DRUM BUS - SHAPER - G-SERIES - 500 SERIES", juce::dontSendNotification);
  titleLabel.setFont(juce::Font(24.0f, juce::Font::bold)); titleLabel.setColour(juce::Label::textColourId, juce::Colours::white); addAndMakeVisible(titleLabel);
  // SHAPER - x 10, w 340
  addToggle("SHAPER_ON","SHAPER ON",20,95,120,22);
  addKnob("SH_ATTACK","ATTACK",20,130,80,80); addKnob("SH_SUSTAIN","SUSTAIN",110,130,80,80); addKnob("SH_SPEED","SPEED",200,130,80,80);
  addKnob("SH_SEP","SEPARATION",20,230,80,80); addKnob("SH_MIX","MIX",110,230,80,80);
  // G COMP - x 360, w 320
  addToggle("COMP_ON","G COMP ON",370,95,120,22); addToggle("COMP_AUTOGAIN","AUTO GAIN",500,95,120,22);
  addKnob("COMP_THRESH","THRESH",370,130,80,80); addKnob("COMP_MAKEUP","MAKEUP",460,130,80,80); addKnob("COMP_MIX","MIX",550,130,80,80);
  addChoice("COMP_ATTACK",370,240,85,26); addChoice("COMP_RELEASE",465,240,85,26); addChoice("COMP_RATIO",560,240,85,26);
  // D-SAT - x 700, w 340 - Drive + Mix only, NO type selector
  addToggle("SAT_ON","D-SAT ON",710,95,110,22); addToggle("SAT_AUTOGAIN","AUTO GAIN",830,95,120,22);
  addKnob("SAT_DRIVE","DRIVE",710,130,80,80); addKnob("SAT_MIX","MIX",800,130,80,80);
  // CLIPPER - x 1060, w 400
  addToggle("CLIP_ON","CLIP ON",1070,95,90,22); addToggle("CLIP_AUTOGAIN","AUTO GAIN",1170,95,120,22);
  addKnob("CLIP_CEIL","CEIL",1070,130,80,80); addKnob("CLIP_MIX","MIX",1160,130,80,80);
  addChoice("CLIP_TYPE",1070,240,110,26);
  vu = std::make_unique<VUMeterDrum>(); addAndMakeVisible(vu.get());
  scope = std::make_unique<WaveScopeDrum>(); scope->bus=&p.bus; addAndMakeVisible(scope.get());
  startTimer(30);
}
DrumBusEditor::~DrumBusEditor(){ stopTimer(); setLookAndFeel(nullptr); }
void DrumBusEditor::timerCallback(){ if(vu) vu->setGR(p.bus.compGR); if(scope) scope->repaint(); }
juce::Slider* DrumBusEditor::addKnob(const juce::String& id, const juce::String& label, int x,int y,int w,int h){
  auto s=std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox); s->setBounds(x,y,w,h); s->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0); addAndMakeVisible(s.get());
  auto a=std::make_unique<Att>(p.apvts,id,*s); sliders.push_back(std::move(s)); satts.push_back(std::move(a));
  auto lb=std::make_unique<juce::Label>(); lb->setText(label, juce::dontSendNotification); lb->setFont(juce::Font(11.0f, juce::Font::bold)); lb->setColour(juce::Label::textColourId, juce::Colours::white); lb->setJustificationType(juce::Justification::centred); lb->setBounds(x-10,y+h, w+20, 16); addAndMakeVisible(lb.get()); knobLabels.push_back(std::move(lb));
  return sliders.back().get();
}
juce::ToggleButton* DrumBusEditor::addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h){ auto t=std::make_unique<juce::ToggleButton>(label); t->setBounds(x,y,w,h); addAndMakeVisible(t.get()); auto a=std::make_unique<BAtt>(p.apvts,id,*t); toggles.push_back(std::move(t)); batts.push_back(std::move(a)); return toggles.back().get(); }
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
  auto panel=[&](int x,int y,int w,int h,juce::Colour col, juce::String title){
    g.setColour(col); g.fillRect(x,y,w,h); g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawRect(x,y,w,h,2);
    g.setColour(juce::Colour(0xff888888));
    for(auto [sx,sy] : {std::pair<int,int>{x+6,y+6},{x+w-10,y+6},{x+6,y+h-10},{x+w-10,y+h-10}}){ g.fillEllipse(sx,sy,6,6); g.setColour(juce::Colours::black); g.drawLine(sx+1,sy+3,sx+5,sy+3,1); g.setColour(juce::Colour(0xff888888)); }
    g.setColour(col.getBrightness()>0.6f? juce::Colours::black : juce::Colours::white); g.setFont(13.0f); g.drawText(title, x+10, y+6, w-20, 16, juce::Justification::left);
  };
  panel(10,60,340,560, juce::Colour(0xff8a2a2a), "TRANSIENT SHAPER");
  panel(360,60,320,560, juce::Colour(0xff4a5a6a), "G-SERIES BUS COMP");
  panel(700,60,340,560, juce::Colour(0xff2b2b2b), "D-SATURATION");
  panel(1060,60,400,560, juce::Colour(0xffc65a1e), "CLIPPER + WAVEFORM");
  g.setColour(juce::Colours::white); g.setFont(11.0f);
  g.drawText("ATTACK",370,268,85,14,juce::Justification::centred); g.drawText("RELEASE",465,268,85,14,juce::Justification::centred); g.drawText("RATIO",560,268,85,14,juce::Justification::centred);
  g.drawText("TYPE",1070,268,110,14,juce::Justification::centred);
  g.setColour(juce::Colours::white); g.setFont(11.0f); g.drawText("G COMP GR METER BELOW", 370, 300, 280, 14, juce::Justification::centred);
  g.drawText("WAVEFORM SCOPE BELOW", 1070, 300, 280, 14, juce::Justification::centred);
  // D-Sat note - no type selector by design
  g.setColour(juce::Colours::white); g.setFont(11.0f); g.drawText("DRIVE + MIX ONLY - NO TYPE", 710, 300, 320, 14, juce::Justification::centred);
}
void DrumBusEditor::resized(){
  titleLabel.setBounds(20,10,700,30);
  if(vu) vu->setBounds(370, 330, 300, 220);
  if(scope) scope->setBounds(1070, 330, 370, 220);
}
juce::AudioProcessorEditor* DrumBusAudioProcessor::createEditor(){ return new DrumBusEditor(*this); }
