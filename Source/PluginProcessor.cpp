#include "PluginProcessor.h"
#include "PluginEditor.h"
static juce::AudioProcessorValueTreeState::ParameterLayout layout(){
  using P = juce::AudioProcessorValueTreeState;
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
  auto F=[&](const char* id,const char* n,float mn,float mx,float d){ v.push_back(std::make_unique<juce::AudioParameterFloat>(id,n,mn,mx,d)); };
  auto B=[&](const char* id,const char* n,bool d){ v.push_back(std::make_unique<juce::AudioParameterBool>(id,n,d)); };
  auto C=[&](const char* id,const char* n,juce::StringArray opts,int d){ v.push_back(std::make_unique<juce::AudioParameterChoice>(id,n,opts,d)); };
  // Transient Shaper - Kilohearts style
  B("SHAPER_ON","Shaper ON",true); F("SH_ATTACK","Shaper Attack",-100,100,0); F("SH_SUSTAIN","Shaper Sustain",-100,100,0); F("SH_SPEED","Shaper Speed",0,100,50); F("SH_SEP","Shaper Separation",0,100,50); F("SH_MIX","Shaper Mix",0,100,100);
  // G Comp - same correct core as Mix Bus
  B("COMP_ON","Comp ON",true); F("COMP_THRESH","Comp Thresh",-30,0,-10); F("COMP_MAKEUP","Comp Makeup",-10,20,0); F("COMP_MIX","Comp Mix",0,100,100);
  C("COMP_ATTACK","Comp Attack",juce::StringArray{"0.1","0.3","1","3","10","30"},4); C("COMP_RELEASE","Comp Release",juce::StringArray{"0.1","0.3","0.6","1.2","AUTO"},4); C("COMP_RATIO","Comp Ratio",juce::StringArray{"2","4","10"},1);
  B("COMP_AUTOGAIN","Comp Auto Gain",true);
  // D-Saturation - Drive + Mix only, no type selector
  B("SAT_ON","Sat ON",true); F("SAT_DRIVE","Sat Drive",0,100,30); F("SAT_MIX","Sat Mix",0,100,100); B("SAT_AUTOGAIN","Sat Auto Gain",true);
  // Clipper - last
  B("CLIP_ON","Clip ON",true); F("CLIP_CEIL","Clip Ceil",-20,0,-1); F("CLIP_MIX","Clip Mix",0,100,100); C("CLIP_TYPE","Clip Type",juce::StringArray{"SOFT","HARD","ASYMM"},0); F("CLIP_METER","CLIP METER dB",0,24,0); B("CLIP_AUTOGAIN","Clip Auto Gain",false);
  return juce::AudioProcessorValueTreeState::ParameterLayout(std::make_move_iterator(v.begin()), std::make_move_iterator(v.end()));
}
DrumBusAudioProcessor::DrumBusAudioProcessor(): AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)), apvts(*this, nullptr, "PARAMS", layout()) { bus.prepare(48000); }
DrumBusAudioProcessor::~DrumBusAudioProcessor(){}
void DrumBusAudioProcessor::prepareToPlay(double sr,int){ bus.prepare(sr); }
void DrumBusAudioProcessor::releaseResources(){}
void DrumBusAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&){
  juce::ScopedNoDenormals d;
  auto G=[&](const char* id) -> float { return *apvts.getRawParameterValue(id); };
  bus.shaperOn=G("SHAPER_ON")>0.5f; bus.shAttack=G("SH_ATTACK"); bus.shSustain=G("SH_SUSTAIN"); bus.shSpeed=G("SH_SPEED"); bus.shSep=G("SH_SEP"); bus.shMix=G("SH_MIX");
  bus.compOn=G("COMP_ON")>0.5f; bus.compThresh=G("COMP_THRESH"); bus.compMakeup=G("COMP_MAKEUP"); bus.compMix=G("COMP_MIX"); bus.compAttackIdx=(int)G("COMP_ATTACK"); bus.compReleaseIdx=(int)G("COMP_RELEASE"); bus.compRatioIdx=(int)G("COMP_RATIO"); bus.compAutoGain=G("COMP_AUTOGAIN")>0.5f;
  bus.satOn=G("SAT_ON")>0.5f; bus.satDrive=G("SAT_DRIVE"); bus.satMix=G("SAT_MIX"); bus.satAutoGain=G("SAT_AUTOGAIN")>0.5f;
  bus.clipOn=G("CLIP_ON")>0.5f; bus.clipCeil=G("CLIP_CEIL"); bus.clipMix=G("CLIP_MIX"); bus.clipType=(int)G("CLIP_TYPE"); bus.clipAutoGain=G("CLIP_AUTOGAIN")>0.5f;
  bus.isRendering = isNonRealtime(); // 4x oversample on RENDER ONLY for D-Sat + Clip
  if(b.getNumChannels()>=2){ auto* dl=b.getWritePointer(0); auto* dr=b.getWritePointer(1); for(int i=0;i<b.getNumSamples();++i) bus.processStereo(dl[i], dr[i]); } else { for(int ch=0;ch<b.getNumChannels();++ch){ auto* data=b.getWritePointer(ch); for(int i=0;i<b.getNumSamples();++i){ float L=data[i], R=data[i]; bus.processStereo(L,R); data[i]=L; } } }
  { float mv=bus.clipMeterDb/24.0f; if(mv<0) mv=0; if(mv>1) mv=1; if(auto* pm=apvts.getParameter("CLIP_METER")) pm->setValueNotifyingHost(mv); }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){ return new DrumBusAudioProcessor(); }
