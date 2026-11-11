#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
struct VUMeterDrum; struct WaveScopeDrum; struct TopVUD;
class DrumBusEditor : public juce::AudioProcessorEditor, public juce::Timer {
public: explicit DrumBusEditor(DrumBusAudioProcessor&); ~DrumBusEditor() override;
  void paint(juce::Graphics&) override; void resized() override; void timerCallback() override;
private:
  DrumBusAudioProcessor& p;
  using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
  using BAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
  using CAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
  std::vector<std::unique_ptr<juce::Slider>> sliders; std::vector<std::unique_ptr<Att>> satts;
  std::vector<std::unique_ptr<juce::ToggleButton>> toggles; std::vector<std::unique_ptr<BAtt>> batts;
  std::vector<std::unique_ptr<juce::ComboBox>> combos; std::vector<std::unique_ptr<CAtt>> catts;
  std::vector<std::unique_ptr<juce::Label>> knobLabels;
  juce::Slider* addKnob(const juce::String& id, const juce::String& label, int x,int y,int w,int h);
  juce::Slider* addKnob(const juce::String& id, const juce::String& label, int x,int y,int w,int h, juce::Colour labelColour);
  juce::ToggleButton* addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h);
  juce::ToggleButton* addToggle(const juce::String& id,const juce::String& label,int x,int y,int w,int h, juce::Colour textColour);
  juce::ComboBox* addChoice(const juce::String& id,int x,int y,int w,int h);
  juce::Label titleLabel;
  std::unique_ptr<VUMeterDrum> vu; std::unique_ptr<WaveScopeDrum> scope;
  std::unique_ptr<juce::TextButton> bypassBtn; std::unique_ptr<BAtt> bypassAtt;
  std::unique_ptr<TopVUD> inVU, outVU;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusEditor)
};
